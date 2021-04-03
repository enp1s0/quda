#include <util_quda.h>
#include <quda_internal.h>

//static auto mySelector = sycl::default_selector();
//static auto mySelector = sycl::host_selector();
static auto mySelector = sycl::cpu_selector();
//static auto mySelector = sycl::gpu_selector();
static sycl::device myDevice;
static sycl::queue *streams;
static const int Nstream = 9;

namespace quda
{

  namespace device
  {

    static bool initialized = false;

    void init(int dev)
    {
      if (initialized) return;
      initialized = true;
      //{
      //auto dh = sycl::device(sycl::host_selector());
      //printfQuda("Name: %s\n", dh.get_info<sycl::info::device::name>().c_str());
      //printfQuda("Version: %s\n", dh.get_info<sycl::info::device::version>().c_str());
      //}

      //if (getVerbosity() >= QUDA_SUMMARIZE) {
      auto ps = sycl::platform::get_platforms();
      printfQuda("SYCL platforms available:\n");
      for(auto p: ps) {
	printfQuda("  %s\n", p.get_info<sycl::info::platform::name>().c_str());
      }

      auto p = sycl::platform(mySelector);
      //auto p = sycl::platform(sycl::host_selector());
      //auto p = ps.back();
      printfQuda("Selected platform: %s\n", p.get_info<sycl::info::platform::name>().c_str());
      printfQuda("  Vendor: %s\n", p.get_info<sycl::info::platform::vendor>().c_str());
      printfQuda("  Version: %s\n", p.get_info<sycl::info::platform::version>().c_str());

      auto ds = p.get_devices();
      auto ndev = ds.size();
      printfQuda("  Number of devices: %lu\n", ndev);
      if(dev >= ndev) {
	errorQuda("Requested device(%i) out of range(%lu)", dev, ndev);
      }

      printfQuda("Selected device number: %i\n", dev);
      myDevice = ds[dev];
      printfQuda("  Name: %s\n", myDevice.get_info<sycl::info::device::name>().c_str());
      printfQuda("  Version: %s\n", myDevice.get_info<sycl::info::device::version>().c_str());
      printfQuda("  Driver version: %s\n", myDevice.get_info<sycl::info::device::driver_version>().c_str());
      printfQuda("  Max compute units: %u\n", myDevice.get_info<sycl::info::device::max_compute_units>());
      printfQuda("  Max work item dimensions: %u\n", myDevice.get_info<sycl::info::device::max_work_item_dimensions>());
      printfQuda("  Max work item sizes: %s\n", str(myDevice.get_info<sycl::info::device::max_work_item_sizes>()).c_str());
      printfQuda("  Max work group size: %lu\n", myDevice.get_info<sycl::info::device::max_work_group_size>());
      printfQuda("  Max num sub groups: %u\n", myDevice.get_info<sycl::info::device::max_num_sub_groups>());
      printfQuda("  Sub group independent forward progress: %s\n", myDevice.get_info<sycl::info::device::sub_group_independent_forward_progress>()?"true":"false");
      printfQuda("  Sub group sizes: %s\n", str(myDevice.get_info<sycl::info::device::sub_group_sizes>()).c_str());
      printfQuda("  Preferred vector width float: %u\n", myDevice.get_info<sycl::info::device::preferred_vector_width_float>());
      printfQuda("  Preferred vector width double: %u\n", myDevice.get_info<sycl::info::device::preferred_vector_width_double>());
      printfQuda("  Native vector width float: %u\n", myDevice.get_info<sycl::info::device::native_vector_width_float>());
      printfQuda("  Native vector width double: %u\n", myDevice.get_info<sycl::info::device::native_vector_width_double>());
      printfQuda("  Max clock frequency: %u MHz\n", myDevice.get_info<sycl::info::device::max_clock_frequency>());
      printfQuda("  Address bits: %u\n", myDevice.get_info<sycl::info::device::address_bits>());
      printfQuda("  Max mem alloc size: %lu\n", myDevice.get_info<sycl::info::device::max_mem_alloc_size>());
      printfQuda("  Max parameter size: %lu\n", myDevice.get_info<sycl::info::device::max_parameter_size>());
      printfQuda("  Mem base addr align: %u\n", myDevice.get_info<sycl::info::device::mem_base_addr_align>());
      printfQuda("  Global mem cache line size: %u\n", myDevice.get_info<sycl::info::device::global_mem_cache_line_size>());
      printfQuda("  Global mem cache size: %lu\n", myDevice.get_info<sycl::info::device::global_mem_cache_size>());
      printfQuda("  Global mem size: %lu\n", myDevice.get_info<sycl::info::device::global_mem_size>());
      printfQuda("  Max constant buffer size: %lu\n", myDevice.get_info<sycl::info::device::max_constant_buffer_size>());
      printfQuda("  max_constant_args: %u\n", myDevice.get_info<sycl::info::device::max_constant_args>());
      printfQuda("  local_mem_size: %lu\n", myDevice.get_info<sycl::info::device::local_mem_size>());
      printfQuda("  error_correction_support: %s\n", myDevice.get_info<sycl::info::device::error_correction_support>()?"true":"false");
    }

    int get_device_count()
    {
      auto p = sycl::platform(mySelector);
      auto ds = p.get_devices();
      auto device_count = ds.size();
      return device_count;
    }

    void print_device_properties()
    {
      auto p = sycl::platform(mySelector);
      auto ds = p.get_devices();
      auto dev_count = ds.size();
      for (int device = 0; device < dev_count; device++) {
	using id = sycl::info::device;
	auto d = ds[device];
        printfQuda("%d - name:                    %s\n", device, d.get_info<id::name>().c_str());
      }
#if 0
        printfQuda("%d - totalGlobalMem:          %lu bytes ( %.2f Gbytes)\n", device, deviceProp.totalGlobalMem,
                   deviceProp.totalGlobalMem / (float)(1024 * 1024 * 1024));
        printfQuda("%d - sharedMemPerBlock:       %lu bytes ( %.2f Kbytes)\n", device, deviceProp.sharedMemPerBlock,
                   deviceProp.sharedMemPerBlock / (float)1024);
        printfQuda("%d - regsPerBlock:            %d\n", device, deviceProp.regsPerBlock);
        printfQuda("%d - warpSize:                %d\n", device, deviceProp.warpSize);
        printfQuda("%d - memPitch:                %lu\n", device, deviceProp.memPitch);
        printfQuda("%d - maxThreadsPerBlock:      %d\n", device, deviceProp.maxThreadsPerBlock);
        printfQuda("%d - maxThreadsDim[0]:        %d\n", device, deviceProp.maxThreadsDim[0]);
        printfQuda("%d - maxThreadsDim[1]:        %d\n", device, deviceProp.maxThreadsDim[1]);
        printfQuda("%d - maxThreadsDim[2]:        %d\n", device, deviceProp.maxThreadsDim[2]);
        printfQuda("%d - maxGridSize[0]:          %d\n", device, deviceProp.maxGridSize[0]);
        printfQuda("%d - maxGridSize[1]:          %d\n", device, deviceProp.maxGridSize[1]);
        printfQuda("%d - maxGridSize[2]:          %d\n", device, deviceProp.maxGridSize[2]);
        printfQuda("%d - totalConstMem:           %lu bytes ( %.2f Kbytes)\n", device, deviceProp.totalConstMem,
                   deviceProp.totalConstMem / (float)1024);
        printfQuda("%d - compute capability:      %d.%d\n", device, deviceProp.major, deviceProp.minor);
        printfQuda("%d - deviceOverlap            %s\n", device, (deviceProp.deviceOverlap ? "true" : "false"));
        printfQuda("%d - multiProcessorCount      %d\n", device, deviceProp.multiProcessorCount);
        printfQuda("%d - kernelExecTimeoutEnabled %s\n", device,
                   (deviceProp.kernelExecTimeoutEnabled ? "true" : "false"));
        printfQuda("%d - integrated               %s\n", device, (deviceProp.integrated ? "true" : "false"));
        printfQuda("%d - canMapHostMemory         %s\n", device, (deviceProp.canMapHostMemory ? "true" : "false"));
        switch (deviceProp.computeMode) {
        case 0: printfQuda("%d - computeMode              0: cudaComputeModeDefault\n", device); break;
        case 1: printfQuda("%d - computeMode              1: cudaComputeModeExclusive\n", device); break;
        case 2: printfQuda("%d - computeMode              2: cudaComputeModeProhibited\n", device); break;
        case 3: printfQuda("%d - computeMode              3: cudaComputeModeExclusiveProcess\n", device); break;
        default: errorQuda("Unknown deviceProp.computeMode.");
        }

        printfQuda("%d - surfaceAlignment         %lu\n", device, deviceProp.surfaceAlignment);
        printfQuda("%d - concurrentKernels        %s\n", device, (deviceProp.concurrentKernels ? "true" : "false"));
        printfQuda("%d - ECCEnabled               %s\n", device, (deviceProp.ECCEnabled ? "true" : "false"));
        printfQuda("%d - pciBusID                 %d\n", device, deviceProp.pciBusID);
        printfQuda("%d - pciDeviceID              %d\n", device, deviceProp.pciDeviceID);
        printfQuda("%d - pciDomainID              %d\n", device, deviceProp.pciDomainID);
        printfQuda("%d - tccDriver                %s\n", device, (deviceProp.tccDriver ? "true" : "false"));
        switch (deviceProp.asyncEngineCount) {
        case 0: printfQuda("%d - asyncEngineCount         1: host -> device only\n", device); break;
        case 1: printfQuda("%d - asyncEngineCount         2: host <-> device\n", device); break;
        case 2: printfQuda("%d - asyncEngineCount         0: not supported\n", device); break;
        default: errorQuda("Unknown deviceProp.asyncEngineCount.");
        }
        printfQuda("%d - unifiedAddressing        %s\n", device, (deviceProp.unifiedAddressing ? "true" : "false"));
        printfQuda("%d - memoryClockRate          %d kilohertz\n", device, deviceProp.memoryClockRate);
        printfQuda("%d - memoryBusWidth           %d bits\n", device, deviceProp.memoryBusWidth);
        printfQuda("%d - l2CacheSize              %d bytes\n", device, deviceProp.l2CacheSize);
        printfQuda("%d - maxThreadsPerMultiProcessor          %d\n\n", device, deviceProp.maxThreadsPerMultiProcessor);
      }
#endif
    }

    void create_context()
    {
      printfQuda("Creating context...");
      auto ctx = sycl::context(myDevice);
      streams = new sycl::queue[Nstream];
      sycl::property_list props{sycl::property::queue::in_order(),
				sycl::property::queue::enable_profiling()};
      for (int i=0; i<Nstream-1; i++) {
        streams[i] = sycl::queue(ctx, myDevice, props);
      }
      streams[Nstream-1] = sycl::queue(ctx, myDevice, props);
      printfQuda(" done\n");
      printfQuda("Testing submit...");
      auto q = streams[Nstream-1];
      q.submit([&](sycl::handler& h) {
		 h.parallel_for<class test>(sycl::range<3>{1,1,1},
					    [=](sycl::item<3> i) {
					      int j = i[0];
					    });
	       });
      printfQuda(" done\n");
    }

    void destroy()
    {
      if (streams) {
        //for (int i=0; i<Nstream; i++) streams[i].~queue();
        delete []streams;
        streams = nullptr;
      }
    }

#if 0
    cudaStream_t get_cuda_stream(const qudaStream_t &stream)
    {
      return streams[stream.idx];
    }
#endif

    sycl::queue get_target_stream(const qudaStream_t &stream)
    {
      //printfQuda("Getting stream %i\n", stream.idx);
      return streams[stream.idx];
    }

    qudaStream_t get_stream(unsigned int i)
    {
      if (i > Nstream) errorQuda("Invalid stream index %u", i);
      qudaStream_t stream;
      stream.idx = i;
      return stream;
      //return qudaStream_t(i);
      // return streams[i];
    }

    qudaStream_t get_default_stream()
    {
      qudaStream_t stream;
      stream.idx = Nstream - 1;
      return stream;
      //return qudaStream_t(Nstream - 1);
      //return streams[Nstream - 1];
    }

    unsigned int get_default_stream_idx()
    {
      return Nstream - 1;
    }

    sycl::queue defaultQueue(void)
    {
      //printfQuda("Getting default queue\n");
      return streams[Nstream-1];
    }

    bool managed_memory_supported()
    {
      auto val = myDevice.has(sycl::aspect::usm_restricted_shared_allocations);
      return val;
    }

    bool shared_memory_atomic_supported()
    {
      auto val = myDevice.has(sycl::aspect::int64_base_atomics);
      // atomic_memory_scope_capabilities
      // work_item, sub_group, work_group, device and system
      return val;
    }

    size_t max_default_shared_memory() {
      static int max_shared_bytes = 0;
      if (max_shared_bytes==0) {
	max_shared_bytes = myDevice.get_info<sycl::info::device::local_mem_size>();
      }
      return max_shared_bytes;
    }

    size_t max_dynamic_shared_memory()
    {
      static int max_shared_bytes = 0;
      if (max_shared_bytes==0) {
	max_shared_bytes = myDevice.get_info<sycl::info::device::local_mem_size>();
      }
      return max_shared_bytes;
    }

    unsigned int max_threads_per_block() {
      //auto val = myDevice.get_info<sycl::info::device::max_work_group_size>();
      auto val = 128;
      return val;
    }

    unsigned int max_threads_per_processor() { // not in portable SYCL
      //auto val = myDevice.get_info<sycl::info::device::max_work_group_size>();
      auto val = 128;
      return 2*val;
    }

    unsigned int max_threads_per_block_dim(int i) {
      auto val = myDevice.get_info<sycl::info::device::max_work_item_sizes>();
      return val[i];
    }

    unsigned int max_grid_size(int i) { // not in portable SYCL?
      auto val = myDevice.get_info<sycl::info::device::max_work_item_sizes>();
      return val[i];
    }

    unsigned int processor_count() {
      auto val = myDevice.get_info<sycl::info::device::max_compute_units>();
      return val;
    }

    unsigned int max_blocks_per_processor() // FIXME
    {
      static int max_blocks_per_sm = 2;
      return max_blocks_per_sm;
    }

    namespace profile {

      void start()
      {
        //cudaProfilerStart();
      }

      void stop()
      {
        //cudaProfilerStop();
      }

    }

  }
}