#ifndef MEMORY_POOL_H
#define MEMORY_POOL_H

#include <array>
#include <vector>
#include <mutex>
#include <memory>
#include <cassert>

template<typename T, size_t BlockSize = 1024>
class MemoryPool {
public:
    MemoryPool() {
        // Pre-allocate first block
        addBlock();
    }

    template<typename... Args>
    std::shared_ptr<T> allocate(Args&&... args) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        // Find a free slot
        if (freeList_.empty()) {
            // All blocks full, add new block
            addBlock();
        }
        
        // Get next free slot
        size_t slotIndex = freeList_.back();
        freeList_.pop_back();
        
        // Create object in place
        T* ptr = new (&storage_[slotIndex]) T(std::forward<Args>(args)...);
        
        // Create custom deleter that returns object to pool
        return std::shared_ptr<T>(ptr, [this, slotIndex](T* p) {
            std::lock_guard<std::mutex> lock(mutex_);
            p->~T();  // Call destructor
            freeList_.push_back(slotIndex);  // Return to free list
        });
    }

    size_t capacity() const {
        return storage_.size();
    }

    size_t available() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return freeList_.size();
    }

private:
    void addBlock() {
        size_t oldSize = storage_.size();
        storage_.resize(oldSize + BlockSize);
        
        // Add new slots to free list in reverse order
        // This ensures we use memory linearly when allocating
        for (size_t i = oldSize + BlockSize; i > oldSize; --i) {
            freeList_.push_back(i - 1);
        }
    }

    std::vector<std::aligned_storage_t<sizeof(T), alignof(T)>> storage_;
    std::vector<size_t> freeList_;
    mutable std::mutex mutex_;
};

#endif // MEMORY_POOL_H