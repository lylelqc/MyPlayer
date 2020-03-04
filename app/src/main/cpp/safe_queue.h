//
// Created by EDZ on 2020/1/9.
//

#ifndef MYPLAYER_SAFE_QUEUE_H
#define MYPLAYER_SAFE_QUEUE_H

#include <queue>
#include <pthread.h>

/**
 * 多线程下，线程安全队列
 */

using namespace std;

template <typename T>
class SafeQueue{
    // 函数指针
    typedef void(* ReleaseCallback)(T *);
    typedef void(* SyncCallback)(queue<T> &);

public:
    SafeQueue(){
        pthread_mutex_init(&lock, 0);
        pthread_cond_init(&cond, 0);
    }

    ~SafeQueue() {
        pthread_mutex_destroy(&lock);
        pthread_cond_destroy(&cond);
    }

    /**
     * 入队
     * @param value
     */
    void push(T value){
        // 加锁
        pthread_mutex_lock(&lock);
        if(work){
            q.push(value);
            pthread_cond_signal(&cond); // 发送通知，队列有数据了
        }else{
            // 需要释放 value
            // 交给调用者释放（回调 -- 函数指针）
            if(releaseCallback){
                releaseCallback(&value);
            }
        }
        pthread_mutex_unlock(&lock);
    }

    /**
     * 出队
     * @param value
     * @return
     */
    int pop(T &value){
        int ret = 0;
        pthread_mutex_lock(&lock);
        while(work && q.empty()){
            // 工作状态且队列为空， 一直等待
            pthread_cond_wait(&cond, &lock);
        }
        if(!q.empty()){
            value = q.front();
            q.pop();
            ret = 1;
        }
        pthread_mutex_unlock(&lock);
        return ret;
    }

    int empty(){
        return q.empty();
    }

    int size(){
        return q.size();
    }

    void clear(){
        pthread_mutex_lock(&lock);
        unsigned int size = q.size();
        for(int i = 0; i < size; ++i){
            T value = q.front();
            // 释放完走下一个
            if(releaseCallback){
                releaseCallback(&value);
            }
            q.pop();
        }
        pthread_mutex_unlock(&lock);
    }

    void setWork(int work){
        pthread_mutex_lock(&lock);
        this->work = work;
        pthread_cond_signal(&cond);
        pthread_mutex_unlock(&lock);
    }

    // 同步
    void sync(){
        pthread_mutex_lock(&lock);
        syncCallback(q);
        pthread_mutex_unlock(&lock);
    }

    void setReleaseCallback(ReleaseCallback releaseCallback){
        this->releaseCallback = releaseCallback;
    }

    void setSyncCallback(SyncCallback syncCallback){
        this->syncCallback = syncCallback;
    }

private:
    queue<T> q;
    pthread_mutex_t lock; // 互斥锁
    int work; // 标记队列是否工作状态
    pthread_cond_t cond;
    ReleaseCallback releaseCallback;
    SyncCallback syncCallback;
};

#endif //MYPLAYER_SAFE_QUEUE_H
