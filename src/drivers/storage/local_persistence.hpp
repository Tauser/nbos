#pragma once

#include <stddef.h>
#include <stdint.h>

namespace ncos::drivers::storage {

enum class LocalPersistenceStatus : uint8_t {
  kOk = 0,
  kNotFound,
  kUnavailable,
  kInvalidArgument,
  kIoError,
};

class LocalPersistence final {
 public:
  bool initialize();
  LocalPersistenceStatus read_blob(const char* ns,
                                   const char* key,
                                   void* out_data,
                                   size_t out_capacity,
                                   size_t* out_size);
  LocalPersistenceStatus write_blob(const char* ns,
                                    const char* key,
                                    const void* data,
                                    size_t size);
  LocalPersistenceStatus erase_key(const char* ns, const char* key);

 private:
  bool initialized_ = false;
  bool available_ = false;
};

}  // namespace ncos::drivers::storage
