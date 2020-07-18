#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <math.h>
#include <string.h>

// QUDA headers
#include <quda.h>
#include <color_spinor_field.h> // convenient quark field container
#include <vector_io.h>
#include <blas_quda.h>
#include <dslash_quda.h>

// External headers
#include <misc.h>
#include <host_utils.h>
#include <command_line_params.h>
#include <dslash_reference.h>

#define MAX(a, b) ((a) > (b) ? (a) : (b))

void display_test_info()
{
  printfQuda("running the following test:\n");
  printfQuda("prec    prec_sloppy   multishift  matpc_type  recon  recon_sloppy solve_type S_dimension T_dimension "
             "Ls_dimension   dslash_type  normalization\n");
  printfQuda(
    "%6s   %6s          %d     %12s     %2s     %2s         %10s %3d/%3d/%3d     %3d         %2d       %14s  %8s\n",
    get_prec_str(prec), get_prec_str(prec_sloppy), multishift, get_matpc_str(matpc_type), get_recon_str(link_recon),
    get_recon_str(link_recon_sloppy), get_solve_str(solve_type), xdim, ydim, zdim, tdim, Lsdim,
    get_dslash_str(dslash_type), get_mass_normalization_str(normalization));

  if (inv_multigrid) {
    printfQuda("MG parameters\n");
    printfQuda(" - number of levels %d\n", mg_levels);
    for (int i = 0; i < mg_levels - 1; i++) {
      printfQuda(" - level %d number of null-space vectors %d\n", i + 1, nvec[i]);
      printfQuda(" - level %d number of pre-smoother applications %d\n", i + 1, nu_pre[i]);
      printfQuda(" - level %d number of post-smoother applications %d\n", i + 1, nu_post[i]);
    }

    printfQuda("MG Eigensolver parameters\n");
    for (int i = 0; i < mg_levels; i++) {
      if (low_mode_check || mg_eig[i]) {
        printfQuda(" - level %d solver mode %s\n", i + 1, get_eig_type_str(mg_eig_type[i]));
        printfQuda(" - level %d spectrum requested %s\n", i + 1, get_eig_spectrum_str(mg_eig_spectrum[i]));
        if (mg_eig_type[i] == QUDA_EIG_BLK_TR_LANCZOS)
          printfQuda(" - eigenvector block size %d\n", mg_eig_block_size[i]);
        printfQuda(" - level %d number of eigenvectors requested n_conv %d\n", i + 1, nvec[i]);
        printfQuda(" - level %d size of eigenvector search space %d\n", i + 1, mg_eig_n_ev[i]);
        printfQuda(" - level %d size of Krylov space %d\n", i + 1, mg_eig_n_kr[i]);
        printfQuda(" - level %d solver tolerance %e\n", i + 1, mg_eig_tol[i]);
        printfQuda(" - level %d convergence required (%s)\n", i + 1, mg_eig_require_convergence[i] ? "true" : "false");
        printfQuda(" - level %d Operator: daggered (%s) , norm-op (%s)\n", i + 1,
                   mg_eig_use_dagger[i] ? "true" : "false", mg_eig_use_normop[i] ? "true" : "false");
        if (mg_eig_use_poly_acc[i]) {
          printfQuda(" - level %d Chebyshev polynomial degree %d\n", i + 1, mg_eig_poly_deg[i]);
          printfQuda(" - level %d Chebyshev polynomial minumum %e\n", i + 1, mg_eig_amin[i]);
          if (mg_eig_amax[i] <= 0)
            printfQuda(" - level %d Chebyshev polynomial maximum will be computed\n", i + 1);
          else
            printfQuda(" - level %d Chebyshev polynomial maximum %e\n", i + 1, mg_eig_amax[i]);
        }
        printfQuda("\n");
      }
    }
  }

  if (inv_deflate) {
    printfQuda("\n   Eigensolver parameters\n");
    printfQuda(" - solver mode %s\n", get_eig_type_str(eig_type));
    printfQuda(" - spectrum requested %s\n", get_eig_spectrum_str(eig_spectrum));
    if (eig_type == QUDA_EIG_BLK_TR_LANCZOS) printfQuda(" - eigenvector block size %d\n", eig_block_size);
    printfQuda(" - number of eigenvectors requested %d\n", eig_n_conv);
    printfQuda(" - size of eigenvector search space %d\n", eig_n_ev);
    printfQuda(" - size of Krylov space %d\n", eig_n_kr);
    printfQuda(" - solver tolerance %e\n", eig_tol);
    printfQuda(" - convergence required (%s)\n", eig_require_convergence ? "true" : "false");
    if (eig_compute_svd) {
      printfQuda(" - Operator: MdagM. Will compute SVD of M\n");
      printfQuda(" - ***********************************************************\n");
      printfQuda(" - **** Overriding any previous choices of operator type. ****\n");
      printfQuda(" - ****    SVD demands normal operator, will use MdagM    ****\n");
      printfQuda(" - ***********************************************************\n");
    } else {
      printfQuda(" - Operator: daggered (%s) , norm-op (%s)\n", eig_use_dagger ? "true" : "false",
                 eig_use_normop ? "true" : "false");
    }
    if (eig_use_poly_acc) {
      printfQuda(" - Chebyshev polynomial degree %d\n", eig_poly_deg);
      printfQuda(" - Chebyshev polynomial minumum %e\n", eig_amin);
      if (eig_amax <= 0)
        printfQuda(" - Chebyshev polynomial maximum will be computed\n");
      else
        printfQuda(" - Chebyshev polynomial maximum %e\n\n", eig_amax);
    }
  }

  printfQuda("Grid partition info:     X  Y  Z  T\n");
  printfQuda("                         %d  %d  %d  %d\n", dimPartitioned(0), dimPartitioned(1), dimPartitioned(2),
             dimPartitioned(3));
}

int main(int argc, char **argv)
{
  setQudaDefaultMgTestParams();
  // Parse command line options
  auto app = make_app();
  add_eigen_option_group(app);
  add_deflation_option_group(app);
  add_eofa_option_group(app);
  add_multigrid_option_group(app);
  add_propagator_option_group(app);
  try {
    app->parse(argc, argv);
  } catch (const CLI::ParseError &e) {
    return app->exit(e);
  }

  if (inv_deflate && inv_multigrid) {
    printfQuda("Error: Cannot use both deflation and multigrid preconditioners on top level solve.\n");
    exit(0);
  }

  // If a value greater than 1 is passed, heavier masses will be constructed
  // and the multi-shift solver will be called
  if (multishift > 1) {
    // set a correct default for the multi-shift solver
    solution_type = QUDA_MATPCDAG_MATPC_SOLUTION;
  }

  // Set values for precisions via the command line.
  setQudaPrecisions();

  // initialize QMP/MPI, QUDA comms grid and RNG (host_utils.cpp)
  initComms(argc, argv, gridsize_from_cmdline);

  if (dslash_type != QUDA_WILSON_DSLASH && dslash_type != QUDA_CLOVER_WILSON_DSLASH
      && dslash_type != QUDA_TWISTED_MASS_DSLASH && dslash_type != QUDA_DOMAIN_WALL_4D_DSLASH
      && dslash_type != QUDA_MOBIUS_DWF_DSLASH && dslash_type != QUDA_MOBIUS_DWF_EOFA_DSLASH
      && dslash_type != QUDA_TWISTED_CLOVER_DSLASH && dslash_type != QUDA_DOMAIN_WALL_DSLASH) {
    printfQuda("dslash_type %d not supported\n", dslash_type);
    exit(0);
  }

  if (inv_multigrid) {
    // Only these fermions are supported with MG
    if (dslash_type != QUDA_WILSON_DSLASH && dslash_type != QUDA_CLOVER_WILSON_DSLASH
        && dslash_type != QUDA_TWISTED_MASS_DSLASH && dslash_type != QUDA_TWISTED_CLOVER_DSLASH) {
      printfQuda("dslash_type %d not supported for MG\n", dslash_type);
      exit(0);
    }

    // Only these solve types are supported with MG
    if (solve_type != QUDA_DIRECT_SOLVE && solve_type != QUDA_DIRECT_PC_SOLVE) {
      printfQuda("Solve_type %d not supported with MG. Please use QUDA_DIRECT_SOLVE or QUDA_DIRECT_PC_SOLVE\n\n",
                 solve_type);
      exit(0);
    }
  }

  // Set QUDA's internal parameters
  QudaGaugeParam gauge_param = newQudaGaugeParam();
  setWilsonGaugeParam(gauge_param);
  QudaInvertParam inv_param = newQudaInvertParam();
  QudaMultigridParam mg_param = newQudaMultigridParam();
  QudaInvertParam mg_inv_param = newQudaInvertParam();
  QudaEigParam mg_eig_param[mg_levels];
  QudaEigParam eig_param = newQudaEigParam();

  if (inv_multigrid) {

    setQudaMgSolveTypes();
    setMultigridInvertParam(inv_param);
    // Set sub structures
    mg_param.invert_param = &mg_inv_param;
    for (int i = 0; i < mg_levels; i++) {
      if (mg_eig[i]) {
        mg_eig_param[i] = newQudaEigParam();
        setMultigridEigParam(mg_eig_param[i], i);
        mg_param.eig_param[i] = &mg_eig_param[i];
      } else {
        mg_param.eig_param[i] = nullptr;
      }
    }
    // Set MG
    setMultigridParam(mg_param);
  } else {
    setInvertParam(inv_param);
  }

  if (inv_deflate) {
    setEigParam(eig_param);
    inv_param.eig_param = &eig_param;
  } else {
    inv_param.eig_param = nullptr;
  }

  // All parameters have been set. Display the parameters via stdout
  display_test_info();

  // Initialize the QUDA library
  initQuda(device);

  // set parameters for the reference Dslash, and prepare fields to be loaded
  if (dslash_type == QUDA_DOMAIN_WALL_DSLASH || dslash_type == QUDA_DOMAIN_WALL_4D_DSLASH
      || dslash_type == QUDA_MOBIUS_DWF_DSLASH || dslash_type == QUDA_MOBIUS_DWF_EOFA_DSLASH) {
    dw_setDims(gauge_param.X, inv_param.Ls);
  } else {
    setDims(gauge_param.X);
  }
  setSpinorSiteSize(24);

  // Allocate host side memory for the gauge field.
  //----------------------------------------------------------------------------
  void *gauge[4];
  // Allocate space on the host (always best to allocate and free in the same scope)
  for (int dir = 0; dir < 4; dir++) gauge[dir] = malloc(V * gauge_site_size * host_gauge_data_type_size);
  constructHostGaugeField(gauge, gauge_param, argc, argv);
  // Load the gauge field to the device
  loadGaugeQuda((void *)gauge, &gauge_param);

  // Allocate host side memory for clover terms if needed.
  //----------------------------------------------------------------------------
  void *clover = nullptr;
  void *clover_inv = nullptr;
  // Allocate space on the host (always best to allocate and free in the same scope)
  if (dslash_type == QUDA_CLOVER_WILSON_DSLASH || dslash_type == QUDA_TWISTED_CLOVER_DSLASH) {
    clover = malloc(V * clover_site_size * host_clover_data_type_size);
    clover_inv = malloc(V * clover_site_size * host_spinor_data_type_size);
    constructHostCloverField(clover, clover_inv, inv_param);
    if (inv_multigrid) {
      // This line ensures that if we need to construct the clover inverse (in either the smoother or the solver) we do so
      if (mg_param.smoother_solve_type[0] == QUDA_DIRECT_PC_SOLVE || solve_type == QUDA_DIRECT_PC_SOLVE) {
        inv_param.solve_type = QUDA_DIRECT_PC_SOLVE;
      }
    }
    // Load the clover terms to the device
    loadCloverQuda(clover, clover_inv, &inv_param);
    if (inv_multigrid) {
      // Restore actual solve_type we want to do
      inv_param.solve_type = solve_type;
    }
  }

  // Now QUDA is initialised and the fields are loaded, we may setup the preconditioner
  void *mg_preconditioner = nullptr;
  if (inv_multigrid) {
    mg_preconditioner = newMultigridQuda(&mg_param);
    inv_param.preconditioner = mg_preconditioner;
  }

  // Compute plaquette as a sanity check
  double plaq[3];
  plaqQuda(plaq);
  printfQuda("Computed plaquette is %e (spatial = %e, temporal = %e)\n", plaq[0], plaq[1], plaq[2]);

  // Vector construct START
  //-----------------------------------------------------------------------------------
  quda::ColorSpinorField *in;
  quda::ColorSpinorField *out;
  quda::ColorSpinorField *quark4D;
  quda::ColorSpinorField *midPoint4D;
  quda::ColorSpinorField *check;
  quda::ColorSpinorParam cs_param;
  constructWilsonTestSpinorParam(&cs_param, &inv_param, &gauge_param);
  quda::ColorSpinorParam cs_param4D;
  // Make 4D params
  QudaInvertParam inv_param4D;
  QudaDslashType dslash_type_orig = dslash_type;
  dslash_type = QUDA_WILSON_DSLASH;
  setInvertParam(inv_param4D);  
  constructWilsonTestSpinorParam(&cs_param4D, &inv_param, &gauge_param);
  
  // Restore dslash type
  dslash_type = dslash_type_orig;
  
  quark4D = quda::ColorSpinorField::Create(cs_param4D);
  midPoint4D = quda::ColorSpinorField::Create(cs_param4D);
  
  out = quda::ColorSpinorField::Create(cs_param);
  check = quda::ColorSpinorField::Create(cs_param);
  
  // Host array for solutions
  void **outProp = (void **)malloc(prop_n_sources * sizeof(void *));
  // QUDA host array for internal checks and malloc
  std::vector<quda::ColorSpinorField *> qudaOutProp(prop_n_sources);
  // Vector construct END
  //-----------------------------------------------------------------------------------

  // QUDA propagator test BEGIN
  //-----------------------------------------------------------------------------------
  double *time = new double[prop_n_sources];
  double *gflops = new double[prop_n_sources];

  printfQuda("Source position: %d %d %d %d\n", prop_source_position[0], prop_source_position[1], prop_source_position[2],
             prop_source_position[3]);

  QudaInvertParam inv_param_smear = newQudaInvertParam();
  // Borrow problem parameters, then adjust
  setInvertParam(inv_param_smear);
  double coeff = prop_source_smear_coeff / (4 * prop_source_smear_steps);
  inv_param_smear.dslash_type = QUDA_LAPLACE_DSLASH;
  inv_param_smear.kappa = coeff;
  inv_param_smear.laplace3D = 3;

  auto *rng = new quda::RNG(quda::LatticeFieldParam(gauge_param), 1234);
  rng->Init();

  for (int dil = 0; dil < prop_n_sources; dil++) {

    // Allocate memory and set pointers
    qudaOutProp[dil] = quda::ColorSpinorField::Create(cs_param);
    outProp[dil] = qudaOutProp[dil]->V();
    in = quda::ColorSpinorField::Create(cs_param);
    
    if (strcmp(prop_source_infile[dil], "") != 0) {

      std::string infile(prop_source_infile[dil]);
      /*
      infile += "_pos";
      for (int i = 0; i < 4; i++) {
        infile += "_";
        infile += std::to_string(prop_source_position[i]);
      }
      infile += "_dilution_";
      infile += std::to_string(dil);
      */
      
      std::vector<quda::ColorSpinorField *> src_ptr;
      src_ptr.reserve(1);
      src_ptr.push_back(in);

      // load the source
      quda::VectorIO io(infile, QUDA_BOOLEAN_TRUE);
      io.load(src_ptr);
    } else {
      constructPointSpinorSource(in->V(), 4, 3, inv_param.cpu_prec, gauge_param.X, dil);
        
      // Gaussian smear the point source.
      performGaussianSmearNStep(in->V(), &inv_param_smear, prop_source_smear_steps);
      
      if (strcmp(prop_source_outfile[0], "") != 0) {
	
        std::string outfile(prop_source_outfile[0]);
        outfile += "_pos";
        for (int i = 0; i < 4; i++) {
          outfile += "_";
          outfile += std::to_string(prop_source_position[i]);
        }
        outfile += "_dilution_";
        outfile += std::to_string(dil);

        std::vector<quda::ColorSpinorField *> src_ptr;
        src_ptr.push_back(in);
	
        // save the source
        quda::VectorIO io(outfile, QUDA_BOOLEAN_TRUE);
        io.save(src_ptr);
      }
    }
    
    // If deflating, preserve the deflation space between solves
    if (inv_deflate) eig_param.preserve_deflation = dil < 12 - 1 ? QUDA_BOOLEAN_TRUE : QUDA_BOOLEAN_FALSE;
    // Perform QUDA inversions
    invertQuda(qudaOutProp[dil]->V(), in->V(), &inv_param);

    time[dil] = inv_param.secs;
    gflops[dil] = inv_param.gflops / inv_param.secs;
    printfQuda("Prop %d done: %d iter / %g secs = %g Gflops\n\n", dil, inv_param.iter, inv_param.secs,
               inv_param.gflops / inv_param.secs);

    // Gaussian smear the point sink.
    performGaussianSmearNStep(qudaOutProp[dil]->V(), &inv_param_smear, prop_sink_smear_steps);

    // Perform host side verification of inversion if requested
    if (verify_results) {
      verifyInversion(out->V(), in->V(), check->V(), gauge_param, inv_param, gauge, clover, clover_inv);
    }

    // Make 4D props
    //make4DMidPointProp(midPoint4D->V(), qudaOutProp[dil]->V(), &inv_param, &inv_param4D, gauge_param.X);
    //qudaOutProp[dil]->PrintVector(0);
    make4DQuarkProp(quark4D->V(), qudaOutProp[dil]->V(), &inv_param, &inv_param4D, gauge_param.X);
    //qudaOutProp[dil]->PrintVector(0);
    
    delete in;
  }
  // QUDA propagator test COMPLETE
  //----------------------------------------------------------------------------

  // Free the multigrid solver
  if (inv_multigrid) destroyMultigridQuda(mg_preconditioner);

  // Compute performance statistics
  Nsrc = 12;
  performanceStats(time, gflops);
  delete[] time;
  delete[] gflops;

  if (strcmp(prop_sink_outfile[0], "") != 0) {
    // Make an array of size 12
    std::vector<quda::ColorSpinorField *> prop_ptr;
    prop_ptr.reserve(12);
    for (int i = 0; i < 12; i++) prop_ptr.push_back(qudaOutProp[i]);

    // save the propagators
    quda::VectorIO io(prop_sink_outfile[0], QUDA_BOOLEAN_TRUE);
    io.save(prop_ptr);
  }

  // Clean up memory allocations
  rng->Release();
  delete rng;
  
  delete out;
  delete check;
  delete midPoint4D;
  delete quark4D;
  
  free(outProp);
  for (int i = 0; i < 12; i++) delete qudaOutProp[i];

  freeGaugeQuda();
  for (int dir = 0; dir < 4; dir++) free(gauge[dir]);

  if (dslash_type == QUDA_CLOVER_WILSON_DSLASH || dslash_type == QUDA_TWISTED_CLOVER_DSLASH) {
    freeCloverQuda();
    if (clover) free(clover);
    if (clover_inv) free(clover_inv);
  }

  // finalize the QUDA library
  endQuda();
  finalizeComms();

  return 0;
}