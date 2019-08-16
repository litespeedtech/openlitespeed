

#include <lsr/ls_offload.h>
#include <thread/workcrew.h>
#include <edio/eventnotifier.h>
#include <edio/multiplexerfactory.h>
#include <log4cxx/logger.h>
#include <lsr/ls_lfqueue.h>
#include <util/autostr.h>

#include <assert.h>

void *process_offload(ls_offload *task)
{
    if (!task->is_canceled)
    {
        LS_DBG_L("[OFFLOAD] worker process Task: %p.\n",  task);
        task->state = LS_OFFLOAD_PROCESSING;
        task->api->perform(task);
        task->state = LS_OFFLOAD_IN_FINISH_QUEUE;
    }
    else
    {
        LS_DBG_L("[OFFLOAD] Task %p is canceled, bypass.\n",  task);
        task->state = LS_OFFLOAD_BYPASS;
    }
    return NULL;
}


struct Offloader : public EventNotifier
{
    ls_lfqueue_t *m_pFinishedQueue;
    WorkCrew     *m_crew;
    AutoStr       m_log_id;

public:
    Offloader();

    ~Offloader();

    void set_log_id(const char *log_id)
    {
        m_log_id.setStr(log_id);
    }
    const char *get_log_id() const
    {   return m_log_id.c_str();    }

    int start(Multiplexer *pMplx, int workers);

    int init();

    int startProcessor(int workers = 1);

    int addJob(ls_offload *task);

    int onNotified(int count);
};


Offloader::Offloader()
    : m_pFinishedQueue(NULL)
    , m_crew(NULL)
{ }


Offloader::~Offloader()
{
    if (m_crew)
        delete m_crew;
    if (m_pFinishedQueue)
        ls_lfqueue_delete(m_pFinishedQueue);
}


int Offloader::init()
{
    if (!m_pFinishedQueue)
    {
        m_pFinishedQueue = ls_lfqueue_new();
        if (!m_pFinishedQueue)
        {
            LS_NOTICE("[%s] Offloader::init() Unable to create Finished queue\n", get_log_id());
            return -1;
        }
    }
    if (!m_crew)
    {
        LS_DBG_L("[%s] Offloader::init() create WorkCrew.\n", get_log_id());
        m_crew = new WorkCrew(this);
        if (!m_crew)
        {
            LS_NOTICE("[%s] Offloader::init() create WorkCrew memory error.\n", get_log_id());
            return -1;
        }

        m_crew->dropPriorityBy(1);
    }
    return 0;
}


int Offloader::addJob(ls_offload *data)
{
    int rc;

    LS_DBG_L("[%s] Offloader::addJob: %p.\n", get_log_id(), data);
    assert(data->task_link == NULL);
    assert(data->api != NULL);
    assert(data->api->on_task_done != NULL);
    assert(data->api->perform != NULL);
    assert(data->api->release != NULL);

    ++data->ref_cnt;
    data->state = LS_OFFLOAD_ENQUEUE;
    rc = m_crew->addJob((ls_lfnodei_t *)data);
    if (rc == -1)
    {
        data->state = LS_OFFLOAD_ENQUEUE_FAIL;
        data->api->release(data);
    }
    return rc;
}


int Offloader::startProcessor(int workers)
{
    return m_crew->startJobProcessor(workers, m_pFinishedQueue,
                                     (WorkCrewProcessFn)process_offload);
}


int Offloader::start(Multiplexer *pMplx, int workers)
{
    if (initNotifier(pMplx) == -1)
        return -1;
    return startProcessor(workers);
}


int Offloader::onNotified(int count)
{
    ls_offload *task;
    LS_DBG_L("[%s] Offloader::onNotified: %d.\n", get_log_id(), count);
    while (!ls_lfqueue_empty(m_pFinishedQueue))
    {
        task = (ls_offload *)ls_lfqueue_get(m_pFinishedQueue);
        if (!task)
        {
            LS_NOTICE("Offloader::onNotified(), Bad Event Object Returned.");
            return LS_FAIL;
        }
        if (!task->is_canceled)
        {
            LS_DBG_L("[%s] on_task_done %p.\n", get_log_id(), task);
            assert(task->state == LS_OFFLOAD_IN_FINISH_QUEUE);
            task->state = LS_OFFLOAD_FINISH_CB;
            task->api->on_task_done(task->param_task_done);
            task->state = LS_OFFLOAD_FINISH_CB_DONE;
        }
        else
        {
            LS_DBG_L("[%s] task %p has been canceled.\n", get_log_id(), task);
            task->state = LS_OFFLOAD_FINISH_CB_BYPASS;
        }
        LS_DBG_L("[%s] release task %p.\n", get_log_id(), task);
        task->api->release(task);
    }
    return LS_OK;

}


struct Offloader *offloader_new(const char *log_id, int workers)
{
    Offloader *offload = new Offloader();
    if (offload->init() == -1)
    {
        delete offload;
        return NULL;
    }
    offload->start(MultiplexerFactory::getMultiplexer(), workers);
    offload->set_log_id(log_id);
    LS_INFO("[%s] created offloader with %d workers.\n", offload->get_log_id(), workers);
    return offload;
}


int offloader_enqueue(struct Offloader * offload, ls_offload *task)
{
    return offload->addJob(task);
}

