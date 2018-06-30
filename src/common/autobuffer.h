//
//  Radio
//
//  A proof of longlink and shortlink concept on mobile end
//

#include <sys/types.h>

class AutoBuffer {
    
public:
    AutoBuffer();
    ~AutoBuffer();
    
    void write(const void* buf, size_t len);
    void read(void* buf, size_t len);
    
    void detach(off_t offset);
    void checkCapacity();
    
    const void* getPtr() const;
    void setLength(size_t length);
    size_t getLength() const;
    size_t getCapacity() const;
    
    bool empty() const;
    size_t left() const;
private:
    unsigned char *ptr_;
    size_t length_;
    size_t capcity_;
    
    void expansion(size_t len);
};
