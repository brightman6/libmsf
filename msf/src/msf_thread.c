


/*
pthread_cond_wait�������pthread_mutex_lock��pthread_mutex_unlock֮�䣬��Ϊ��Ҫ���ݹ���������״̬�������Ƿ�Ҫ�ȴ�����Ϊ�˲���
Զ�ȴ���ȥ���Ա���Ҫ��lock/unlock����

����������״̬�ı��������lock/unlock�Ĺ���
pthread_cond_signal�����Է���pthread_mutex_lock��pthread_mutex_unlock֮�䣬Ҳ���Է���pthread_mutex_lock��pthread_mutex_unlock֮�󣬵��Ǹ��и�ȱ�㡣

   


֮��

pthread_mutex_lock
xxxxxxx
pthread_cond_signal
pthread_mutex_unlock

ȱ�㣺��ĳЩ�̵߳�ʵ���У�����ɵȴ��̴߳��ں��л��ѣ�����cond_signal)Ȼ���ֻص��ں˿ռ䣨��Ϊcond_wait���غ����ԭ�Ӽ�������Ϊ��������һ��
һ�ػ������ܵ����⡣������LinuxThreads����NPTL���棬�Ͳ�����������⣬��Ϊ��Linux �߳��У����������У��ֱ���cond_wait���к�mutex_lock���У� 
cond_signalֻ�����̴߳�cond_wait�����Ƶ�mutex_lock���У������÷��ص��û��ռ䣬���������ܵ���ġ�
������Linux���Ƽ�ʹ������ģʽ��


֮��

pthread_mutex_lock
xxxxxxx
pthread_mutex_unlock
pthread_cond_signal

�ŵ㣺�������֮ǰ˵���Ǹ�Ǳ�ڵ�������ģ���Ϊ��signal֮ǰ���Ѿ��ͷ�����

ȱ�㣺���unlock��signal֮ǰ���и������ȼ����߳�����mutex�ϵȴ��Ļ�����ô��������ȼ����߳̾ͻ���ռ�����ȼ����̣߳�cond_wait���߳�)��
����������ķ��м��ģʽ���ǲ�����ֵġ�

���ԣ���Linux�����pthread_cond_signal���м䣬���ӱ�̹�����˵���������ֶ�����
*/


/*
    pthread_cond_wait(pthread_cond_t *cond, pthread_mutex_t*mutex)��������Ĳ���mutex���ڱ�����������Ϊ�����ڵ���pthread_cond_waitʱ��
����������������Ǿͽ������������ǽ�����������ڼ䣬������������ı��˵Ļ��������Ǿ�©���������������Ϊ����̻߳�û�зŵ��ȴ������ϣ�
���Ե���pthread_cond_waitǰҪ������������������pthread_mutex_lock()��pthread_cond_wait�ڰ��̷߳Ž��������к��Զ���mutex���н�����
ʹ�������߳̿��Ի�ü�����Ȩ�������������̲߳��ܶ��ٽ���Դ���з��ʲ����ʵ���ʱ������������Ľ��̡���pthread_cond_wait���ص�ʱ��
���Զ���mutex������
ʵ���ϱߴ���ļӽ����������£�
/ ***********pthread_cond_wait()��ʹ�÷���********** /
pthread_mutex_lock(&qlock); / *lock* /
pthread_cond_wait(&qready, &qlock); / *block-->unlock-->wait() return-->lock* /
pthread_mutex_unlock(&qlock); / *unlock* /

*/
//pthread_cond_wait�ڲ�ִ��������:��������--����--�����̵߳ȴ����У���������(�����߳�ngx_thread_cond_signal)���ٴμ�����Ȼ�󷵻أ�
//Ϊʲô�ڵ��øú�����ʱ�����Ҫ�ȼ�����ԭ������pthread_cond_wait�ڲ���������״̬�и����̣���������������߳�cond signal�����߳�
//��ⲻ�����ź�signal�����������������ñ��߳̽���wait�̵߳ȴ����к󣬲����������߳�cond signal���ѱ��̣߳��Ϳ��Ա���©���ź�


#if 0
ngx_int_t
ngx_thread_mutex_create(ngx_thread_mutex_t *mtx, ngx_log_t *log)
{
    ngx_err_t            err;
    pthread_mutexattr_t  attr;

    err = pthread_mutexattr_init(&attr);
    if (err != 0) {
        ngx_log_error(NGX_LOG_EMERG, log, err,
                      "pthread_mutexattr_init() failed");
        return NGX_ERROR;
    }

    /*
    ���mutex������ PTHREAD_MUTEX_ERRORCHECK����ô�����д����顣���һ���߳���ͼ��һ���Ѿ���ס��mutex����relock��������һ
    ���������һ���̶߳�δ�����Ļ��Ѿ�unlock��mutex�������unlock������������һ������ 
     */
    err = pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_ERRORCHECK);
    if (err != 0) {
        ngx_log_error(NGX_LOG_EMERG, log, err,
                      "pthread_mutexattr_settype"
                      "(PTHREAD_MUTEX_ERRORCHECK) failed");
        return NGX_ERROR;
    }

    err = pthread_mutex_init(mtx, &attr);
    if (err != 0) {
        ngx_log_error(NGX_LOG_EMERG, log, err,
                      "pthread_mutex_init() failed");
        return NGX_ERROR;
    }

    err = pthread_mutexattr_destroy(&attr);
    if (err != 0) {
        ngx_log_error(NGX_LOG_ALERT, log, err,
                      "pthread_mutexattr_destroy() failed");
    }

    ngx_log_debug1(NGX_LOG_DEBUG_CORE, log, 0,
                   "pthread_mutex_init(%p)", mtx);
    return NGX_OK;
}


ngx_int_t
ngx_thread_mutex_destroy(ngx_thread_mutex_t *mtx, ngx_log_t *log)
{
    ngx_err_t  err;

    err = pthread_mutex_destroy(mtx);
    if (err != 0) {
        ngx_log_error(NGX_LOG_ALERT, log, err,
                      "pthread_mutex_destroy() failed");
        return NGX_ERROR;
    }

    ngx_log_debug1(NGX_LOG_DEBUG_CORE, log, 0,
                   "pthread_mutex_destroy(%p)", mtx);
    return NGX_OK;
}


ngx_int_t
ngx_thread_mutex_lock(ngx_thread_mutex_t *mtx, ngx_log_t *log)
{
    ngx_err_t  err;

    ngx_log_debug1(NGX_LOG_DEBUG_CORE, log, 0,
                   "pthread_mutex_lock(%p) enter", mtx);

    err = pthread_mutex_lock(mtx);
    if (err == 0) {
        return NGX_OK;
    }

    ngx_log_error(NGX_LOG_ALERT, log, err, "pthread_mutex_lock() failed");

    return NGX_ERROR;
}


ngx_int_t
ngx_thread_mutex_unlock(ngx_thread_mutex_t *mtx, ngx_log_t *log)
{
    ngx_err_t  err;

    err = pthread_mutex_unlock(mtx);

#if 0
    ngx_time_update();
#endif

    if (err == 0) {
        ngx_log_debug1(NGX_LOG_DEBUG_CORE, log, 0,
                       "pthread_mutex_unlock(%p) exit", mtx);
        return NGX_OK;
    }

    ngx_log_error(NGX_LOG_ALERT, log, err, "pthread_mutex_unlock() failed");

    return NGX_ERROR;
}




#endif