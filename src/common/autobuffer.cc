//
//  Radio
//
//  A proof of longlink and shortlink concept on mobile end
//

#include "autobuffer.h"

#include <stdlib.h>
#include <string>

#define EXPANSION_FACTOR 8 * 1024
#define EXPANSION_AMOUNT 16 * 1024

AutoBuffer::AutoBuffer() {
    ptr_ = NULL;
    length_ = 0;
    capcity_ = EXPANSION_AMOUNT;
}

AutoBuffer::~AutoBuffer() {
    if (NULL != ptr_) {
        free(ptr_);
    }
}

void AutoBuffer::write(const void* buf, size_t len) {
    checkCapacity();
    
    memcpy(ptr_ + getLength(), buf, len);
    setLength(getLength() + len);
}

void AutoBuffer::read(void* buf, size_t len) {
    if (NULL == ptr_) {
        return;
    }

    size_t read_len = std::min(len, getLength());
    memcpy(buf, ptr_, read_len);
    detach(-read_len);
}

void AutoBuffer::detach(off_t offset) {
    if (NULL == ptr_) {
        return;
    }

    if (0 <= offset) {
        return;
    }
    
    size_t move_len = -offset;
    move_len = std::min(move_len, getLength());
    
    memmove(ptr_, ptr_ + move_len, getLength() - move_len);
    setLength(getLength() - move_len);
}

void AutoBuffer::checkCapacity() {
    if (NULL == ptr_) {
        return;
    }
    
    if (EXPANSION_FACTOR > getCapacity() - getLength()) {
        return;
    }
    
    expansion(EXPANSION_AMOUNT);
}

void AutoBuffer::expansion(size_t len) {
    size_t expansion_capacity = getCapacity() + len;
    void *realloc_ptr = realloc(ptr_, expansion_capacity);
    
    if (NULL == realloc_ptr) {
        free(ptr_);
    }
    
    ptr_ = (unsigned char *)realloc_ptr;
    
    memset(ptr_ + capcity_, 0, expansion_capacity);
    
    capcity_ = getCapacity() + len;
}

bool AutoBuffer::empty() const {
    return 0 == getLength();
}

size_t AutoBuffer::left() const {
    return capcity_ - getLength();
}

const void* AutoBuffer::getPtr() const {
    return (const void *)ptr_;
}

void AutoBuffer::setLength(size_t length) {
    length_ = length;
}

size_t AutoBuffer::getLength() const {
    return length_;
}

size_t AutoBuffer::getCapacity() const {
    return capcity_;
}
