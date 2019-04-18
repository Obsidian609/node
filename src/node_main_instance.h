#ifndef SRC_NODE_MAIN_INSTANCE_H_
#define SRC_NODE_MAIN_INSTANCE_H_

#if defined(NODE_WANT_INTERNALS) && NODE_WANT_INTERNALS

#include <cstddef>
#include "node.h"
#include "util.h"
#include "uv.h"
#include "v8.h"

namespace node {

// TODO(joyeecheung): align this with the Worker/WorkerThreadData class.
// We may be able to create an abstract class to reuse some of the routines.
class NodeMainInstance {
 public:
  // An array of indexes that can be used to deserialize data from a V8
  // snapshot.
  struct IndexArray {
    const size_t* data;
    size_t length;

    size_t Get(size_t index) const {
      DCHECK_LT(index, length);
      return data[index];
    }
  };

  // To create a main instance that does not own the isoalte,
  // The caller needs to do:
  //
  //   Isolate* isolate = Isolate::Allocate();
  //   platform->RegisterIsolate(isolate, loop);
  //   isolate->Initialize(...);
  //   isolate->Enter();
  //   NodeMainInstance* main_instance =
  //       NodeMainInstance::Create(isolate, loop, args, exec_args);
  //
  // When tearing it down:
  //
  //   main_instance->Cleanup();  // While the isolate is entered
  //   isolate->Exit();
  //   isolate->Dispose();
  //   platform->UnregisterIsolate(isolate);
  //
  // After calling Dispose() the main_instance is no longer accessible.
  static NodeMainInstance* Create(v8::Isolate* isolate,
                                  uv_loop_t* event_loop,
                                  MultiIsolatePlatform* platform,
                                  const std::vector<std::string>& args,
                                  const std::vector<std::string>& exec_args);
  void Dispose();

  // Create a main instance that owns the isolate
  NodeMainInstance(v8::Isolate::CreateParams* params,
                   uv_loop_t* event_loop,
                   MultiIsolatePlatform* platform,
                   const std::vector<std::string>& args,
                   const std::vector<std::string>& exec_args,
                   const IndexArray* per_isolate_data_indexes = nullptr);
  ~NodeMainInstance();

  // Start running the Node.js instances, return the exit code when finished.
  int Run();

  IsolateData* isolate_data() { return isolate_data_.get(); }

  // TODO(joyeecheung): align this with the CreateEnvironment exposed in node.h
  // and the environment creation routine in workers somehow.
  std::unique_ptr<Environment> CreateMainEnvironment(int* exit_code);

  // If nullptr is returned, the binary is not built with embedded
  // snapshot.
  static const IndexArray* GetIsolateDataIndexes();
  static v8::StartupData* GetEmbeddedSnapshotBlob();

 private:
  NodeMainInstance(const NodeMainInstance&) = delete;
  NodeMainInstance& operator=(const NodeMainInstance&) = delete;
  NodeMainInstance(NodeMainInstance&&) = delete;
  NodeMainInstance& operator=(NodeMainInstance&&) = delete;

  NodeMainInstance(v8::Isolate* isolate,
                   uv_loop_t* event_loop,
                   MultiIsolatePlatform* platform,
                   const std::vector<std::string>& args,
                   const std::vector<std::string>& exec_args);

  std::vector<std::string> args_;
  std::vector<std::string> exec_args_;
  std::unique_ptr<ArrayBufferAllocator> array_buffer_allocator_;
  v8::Isolate* isolate_;
  MultiIsolatePlatform* platform_;
  std::unique_ptr<IsolateData> isolate_data_;
  bool owns_isolate_ = false;
  bool deserialize_mode_ = false;
};

}  // namespace node

#endif  // defined(NODE_WANT_INTERNALS) && NODE_WANT_INTERNALS
#endif  // SRC_NODE_MAIN_INSTANCE_H_
