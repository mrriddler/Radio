//
//  Radio
//
//  A proof of longlink and shortlink concept on mobile end
//

class ActivityMonitor {
public:
    static ActivityMonitor& singleton();
    bool isForeground() const;
};
