//
//  Singleton.h
//  ProjectSLG
//
//  Created by shenxinxing on 15/12/16.
//
//

#ifndef Singleton_h
#define Singleton_h

#include "cocos2d.h"

template <class T>
class Singleton
{
public:
    Singleton(void){}
    virtual ~Singleton(){}
    static inline std::shared_ptr<T> getInstance();
private:
    CC_DISALLOW_COPY_AND_ASSIGN(Singleton)
    
private:
    static std::shared_ptr<T> ms_instance;
    static std::once_flag only_one;
};

template <class T>
std::once_flag Singleton<T>::only_one;

template <class T>
std::shared_ptr<T> Singleton<T>::ms_instance = nullptr;

template <class T>
inline std::shared_ptr<T> Singleton<T>::getInstance(){
    std::call_once(Singleton<T>::only_one,
                   []()
                    {
                        Singleton<T>::ms_instance.reset(new T);
                    });
    return Singleton<T>::ms_instance;
}

#endif /* Singleton_h */
