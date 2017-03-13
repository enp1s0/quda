#include <transfer.h>
#include <color_spinor_field.h>
#include <color_spinor_field_order.h>
#include <gauge_field.h>
#include <gauge_field_order.h>
#include <complex_quda.h>
#include <index_helper.cuh>
#include <gamma.cuh>
#include <blas_magma.h>
#include <coarse_op.cuh>

namespace quda {

  template <typename Float, QudaFieldOrder csOrder, QudaGaugeFieldOrder gOrder, 
            int fineColor, int fineSpin, int coarseColor, int coarseSpin>
  void calculateYcoarse(GaugeField &Y, GaugeField &X, GaugeField &Xinv, GaugeField &Yhat,
			ColorSpinorField &uv, const Transfer &T, const GaugeField &g, const GaugeField &clover,
			const GaugeField &cloverInv, double kappa, double mu, double mu_factor, QudaDiracType dirac, QudaMatPCType matpc) {

    typedef typename colorspinor::FieldOrderCB<Float,fineSpin,fineColor,coarseColor,csOrder> F;
    typedef typename colorspinor::FieldOrderCB<Float,2*fineSpin,fineColor,coarseColor,csOrder> F2;
    typedef typename gauge::FieldOrder<Float,fineColor*fineSpin,fineSpin,gOrder> gFine;
    typedef typename gauge::FieldOrder<Float,coarseColor*coarseSpin,coarseSpin,gOrder> gCoarse;

    const ColorSpinorField &v = T.Vectors(Y.Location());
    int dummy = 0;
    v.exchangeGhost(QUDA_INVALID_PARITY, dummy);

    F vAccessor(const_cast<ColorSpinorField&>(v));
    F2 uvAccessor(const_cast<ColorSpinorField&>(uv));
    gFine gAccessor(const_cast<GaugeField&>(g));
    gFine cAccessor(const_cast<GaugeField&>(clover));
    gFine cInvAccessor(const_cast<GaugeField&>(cloverInv));
    gCoarse yAccessor(const_cast<GaugeField&>(Y));
    gCoarse xAccessor(const_cast<GaugeField&>(X)); 
    gCoarse xInvAccessor(const_cast<GaugeField&>(Xinv));

    calculateY<true,Float,fineSpin,fineColor,coarseSpin,coarseColor,gOrder>
      (yAccessor, xAccessor, xInvAccessor, uvAccessor, vAccessor, vAccessor, gAccessor, cAccessor, cInvAccessor,
       Y, X, Xinv, Yhat, const_cast<ColorSpinorField&>(v), v, kappa, mu, mu_factor, dirac, matpc);
  }


  // template on the number of coarse degrees of freedom
  template <typename Float, QudaFieldOrder csOrder, QudaGaugeFieldOrder gOrder, int fineColor, int fineSpin>
  void calculateYcoarse(GaugeField &Y, GaugeField &X, GaugeField &Xinv, GaugeField &Yhat,
			ColorSpinorField &uv, const Transfer &T, const GaugeField &g, const GaugeField &clover,
			const GaugeField &cloverInv, double kappa, double mu, double mu_factor, QudaDiracType dirac, QudaMatPCType matpc) {
    if (T.Vectors().Nspin()/T.Spin_bs() != 2) 
      errorQuda("Unsupported number of coarse spins %d\n",T.Vectors().Nspin()/T.Spin_bs());
    const int coarseSpin = 2;
    const int coarseColor = Y.Ncolor() / coarseSpin;

    if (coarseColor == 2) { 
      calculateYcoarse<Float,csOrder,gOrder,fineColor,fineSpin,2,coarseSpin>(Y, X, Xinv, Yhat, uv, T, g, clover, cloverInv, kappa, mu, mu_factor, dirac, matpc);
#if 0
    } else if (coarseColor == 8) {
      calculateYcoarse<Float,csOrder,gOrder,fineColor,fineSpin,8,coarseSpin>(Y, X, Xinv, Yhat, uv, T, g, clover, cloverInv, kappa, mu, mu_factor, dirac, matpc);
    } else if (coarseColor == 16) {
      calculateYcoarse<Float,csOrder,gOrder,fineColor,fineSpin,16,coarseSpin>(Y, X, Xinv, Yhat, uv, T, g, clover, cloverInv, kappa, mu, mu_factor, dirac, matpc);
#endif
    } else if (coarseColor == 24) {
      calculateYcoarse<Float,csOrder,gOrder,fineColor,fineSpin,24,coarseSpin>(Y, X, Xinv, Yhat, uv, T, g, clover, cloverInv, kappa, mu, mu_factor, dirac, matpc);
    } else if (coarseColor == 32) {
      calculateYcoarse<Float,csOrder,gOrder,fineColor,fineSpin,32,coarseSpin>(Y, X, Xinv, Yhat, uv, T, g, clover, cloverInv, kappa, mu, mu_factor, dirac, matpc);
    } else {
      errorQuda("Unsupported number of coarse dof %d\n", Y.Ncolor());
    }
  }

  // template on fine spin
  template <typename Float, QudaFieldOrder csOrder, QudaGaugeFieldOrder gOrder, int fineColor>
  void calculateYcoarse(GaugeField &Y, GaugeField &X, GaugeField &Xinv, GaugeField &Yhat,
			ColorSpinorField &uv, const Transfer &T, const GaugeField &g, const GaugeField &clover,
			const GaugeField &cloverInv, double kappa, double mu, double mu_factor, QudaDiracType dirac, QudaMatPCType matpc) {
    if (T.Vectors().Nspin() == 2) {
      calculateYcoarse<Float,csOrder,gOrder,fineColor,2>(Y, X, Xinv, Yhat, uv, T, g, clover, cloverInv, kappa, mu, mu_factor, dirac, matpc);
    } else {
      errorQuda("Unsupported number of spins %d\n", T.Vectors().Nspin());
    }
  }

  // template on fine colors
  template <typename Float, QudaFieldOrder csOrder, QudaGaugeFieldOrder gOrder>
  void calculateYcoarse(GaugeField &Y, GaugeField &X, GaugeField &Xinv, GaugeField &Yhat,
			ColorSpinorField &uv, const Transfer &T, const GaugeField &g, const GaugeField &clover,
			const GaugeField &cloverInv, double kappa, double mu, double mu_factor, QudaDiracType dirac, QudaMatPCType matpc) {
    if (g.Ncolor()/T.Vectors().Nspin() == 2) {
      calculateYcoarse<Float,csOrder,gOrder,2>(Y, X, Xinv, Yhat, uv, T, g, clover, cloverInv, kappa, mu, mu_factor, dirac, matpc);
#if 0
    } else if (g.Ncolor()/T.Vectors().Nspin() == 8) {
      calculateYcoarse<Float,csOrder,gOrder,8>(Y, X, Xinv, Yhat, uv, T, g, clover, cloverInv, kappa, mu, mu_factor, dirac, matpc);
    } else if (g.Ncolor()/T.Vectors().Nspin() == 16) {
      calculateYcoarse<Float,csOrder,gOrder,16>(Y, X, Xinv, Yhat, uv, T, g, clover, cloverInv, kappa, mu, mu_factor, dirac, matpc);
#endif
    } else if (g.Ncolor()/T.Vectors().Nspin() == 24) {
      calculateYcoarse<Float,csOrder,gOrder,24>(Y, X, Xinv, Yhat, uv, T, g, clover, cloverInv, kappa, mu, mu_factor, dirac, matpc);
    } else if (g.Ncolor()/T.Vectors().Nspin() == 32) {
      calculateYcoarse<Float,csOrder,gOrder,32>(Y, X, Xinv, Yhat, uv, T, g, clover, cloverInv, kappa, mu, mu_factor, dirac, matpc);
    } else {
      errorQuda("Unsupported number of colors %d\n", g.Ncolor());
    }
  }

  template <typename Float, QudaFieldOrder csOrder>
  void calculateYcoarse(GaugeField &Y, GaugeField &X, GaugeField &Xinv, GaugeField &Yhat,
			ColorSpinorField &uv, const Transfer &T, const GaugeField &g, const GaugeField &clover,
			const GaugeField &cloverInv, double kappa, double mu, double mu_factor, QudaDiracType dirac, QudaMatPCType matpc) {
    if (g.FieldOrder() == QUDA_QDP_GAUGE_ORDER) {
      calculateYcoarse<Float,csOrder,QUDA_QDP_GAUGE_ORDER>(Y, X, Xinv, Yhat, uv, T, g, clover, cloverInv, kappa, mu, mu_factor, dirac, matpc);
    } else if (g.FieldOrder() == QUDA_FLOAT2_GAUGE_ORDER) {
      calculateYcoarse<Float,csOrder,QUDA_FLOAT2_GAUGE_ORDER>(Y, X, Xinv, Yhat, uv, T, g, clover, cloverInv, kappa, mu, mu_factor, dirac, matpc);
    } else {
      errorQuda("Unsupported field order %d\n", g.FieldOrder());
    }
  }

  template <typename Float>
  void calculateYcoarse(GaugeField &Y, GaugeField &X, GaugeField &Xinv, GaugeField &Yhat,
			ColorSpinorField &uv, const Transfer &T, const GaugeField &g, const GaugeField &clover,
			const GaugeField &cloverInv, double kappa, double mu, double mu_factor, QudaDiracType dirac, QudaMatPCType matpc) {
    if (T.Vectors(Y.Location()).FieldOrder() == QUDA_SPACE_SPIN_COLOR_FIELD_ORDER) {
      calculateYcoarse<Float,QUDA_SPACE_SPIN_COLOR_FIELD_ORDER>(Y, X, Xinv, Yhat, uv, T, g, clover, cloverInv, kappa, mu, mu_factor, dirac, matpc);
    } else if (T.Vectors(Y.Location()).FieldOrder() == QUDA_FLOAT2_FIELD_ORDER) {
      calculateYcoarse<Float,QUDA_FLOAT2_FIELD_ORDER>(Y, X, Xinv, Yhat, uv, T, g, clover, cloverInv, kappa, mu, mu_factor, dirac, matpc);
    } else {
      errorQuda("Unsupported field order %d\n", T.Vectors(Y.Location()).FieldOrder());
    }
  }

  //Does the heavy lifting of creating the coarse color matrices Y
  void calculateYcoarse(GaugeField &Y, GaugeField &X, GaugeField &Xinv, GaugeField &Yhat, ColorSpinorField &uv,
			const Transfer &T, const GaugeField &g, const GaugeField &clover, const GaugeField &cloverInv,
			double kappa, double mu, double mu_factor, QudaDiracType dirac, QudaMatPCType matpc) {
    if (X.Precision() != Y.Precision() || Y.Precision() != uv.Precision() || 
        Y.Precision() != T.Vectors().Precision() || Y.Precision() != g.Precision())
      errorQuda("Unsupported precision mix");

    printfQuda("Computing Y field......\n");
    if (Y.Precision() == QUDA_DOUBLE_PRECISION) {
#ifdef GPU_MULTIGRID_DOUBLE
      calculateYcoarse<double>(Y, X, Xinv, Yhat, uv, T, g, clover, cloverInv, kappa, mu, mu_factor, dirac, matpc);
#else
      errorQuda("Double precision multigrid has not been enabled");
#endif
    } else if (Y.Precision() == QUDA_SINGLE_PRECISION) {
      calculateYcoarse<float>(Y, X, Xinv, Yhat, uv, T, g, clover, cloverInv, kappa, mu, mu_factor, dirac, matpc);
    } else {
      errorQuda("Unsupported precision %d\n", Y.Precision());
    }
    printfQuda("....done computing Y field\n");
  }

  //Calculates the coarse color matrix and puts the result in Y.
  //N.B. Assumes Y, X have been allocated.
  void CoarseCoarseOp(GaugeField &Y, GaugeField &X, GaugeField &Xinv, GaugeField &Yhat, const Transfer &T,
		      const GaugeField &gauge, const GaugeField &clover, const GaugeField &cloverInv,
		      double kappa, double mu, double mu_factor, QudaDiracType dirac, QudaMatPCType matpc) {

    QudaPrecision precision = Y.Precision();
    QudaFieldLocation location = Location(X, Y, Xinv, Yhat, gauge, clover, cloverInv);

    //Create a field UV which holds U*V.  Has the same similar
    //structure to V but double the number of spins so we can store
    //the four distinct block chiral multiplications in a single UV
    //computation.
    ColorSpinorParam UVparam(T.Vectors(location));
    UVparam.create = QUDA_ZERO_FIELD_CREATE;
    UVparam.location = location;
    UVparam.nSpin *= 2; // so nSpin == 4

    ColorSpinorField *uv = ColorSpinorField::Create(UVparam);

    calculateYcoarse(Y, X, Xinv, Yhat, *uv, T, gauge, clover, cloverInv, kappa, mu, mu_factor, dirac, matpc);

    delete uv;
  }
  
} //namespace quda
