package sjtu.sdic.mapreduce.core;

import com.sun.xml.internal.ws.policy.privateutil.PolicyUtils;
import sjtu.sdic.mapreduce.common.Channel;
import sjtu.sdic.mapreduce.common.DoTaskArgs;
import sjtu.sdic.mapreduce.common.JobPhase;
import sjtu.sdic.mapreduce.common.Utils;
import sjtu.sdic.mapreduce.rpc.Call;

import java.io.IOException;
import java.util.ArrayList;
import java.util.Collections;
import java.util.List;
import java.util.Vector;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.CopyOnWriteArrayList;

/**
 * Created by Cachhe on 2019/4/22.
 */
public class Scheduler {

    /**
     * schedule() starts and waits for all tasks in the given phase (mapPhase
     * or reducePhase). the mapFiles argument holds the names of the files that
     * are the inputs to the map phase, one per map task. nReduce is the
     * number of reduce tasks. the registerChan argument yields a stream
     * of registered workers; each item is the worker's RPC address,
     * suitable for passing to {@link Call}. registerChan will yield all
     * existing registered workers (if any) and new ones as they register.
     *
     * @param jobName job name
     * @param mapFiles files' name (if in same dir, it's also the files' path)
     * @param nReduce the number of reduce task that will be run ("R" in the paper)
     * @param phase MAP or REDUCE
     * @param registerChan register info channel
     */
    public static void schedule(String jobName, String[] mapFiles, int nReduce, JobPhase phase, Channel<String> registerChan) {
        int nTasks = -1; // number of map or reduce tasks
        int nOther = -1; // number of inputs (for reduce) or outputs (for map)
        switch (phase) {
            case MAP_PHASE:
                nTasks = mapFiles.length;
                nOther = nReduce;
                break;
            case REDUCE_PHASE:
                nTasks = nReduce;
                nOther = mapFiles.length;
                break;
        }

        System.out.println(String.format("Schedule: %d %s tasks (%d I/Os)", nTasks, phase, nOther));

        //TODO:
        // All ntasks tasks have to be scheduled on workers. Once all tasks
        // have completed successfully, schedule() should return.
        // Your code here (Part III, Part IV).
        CountDownLatch downLatch = new CountDownLatch(nTasks);

        //vector: multi-thread safe
        CopyOnWriteArrayList<Boolean> task_flag = new CopyOnWriteArrayList();
        for(int i = 0; i < nTasks; i++){
            task_flag.add(false);
        }

        try {
            boolean finish_flag = false;
            while(!finish_flag){
                finish_flag = true;
                for(int i = 0; i < nTasks; i++){
                    if(task_flag.get(i))
                        continue;
                    finish_flag = false;

                    DoTaskArgs taskArgs = new DoTaskArgs(jobName, mapFiles[i], phase, i, nOther);
                    Task t = new Task(registerChan.read(), downLatch, registerChan, task_flag, taskArgs);
                    Thread do_task = new Thread(t);
                    do_task.start();
                }
                //wait for all task have completed, then return
                downLatch.await();
            }
        }
        catch (InterruptedException i){
            i.printStackTrace();
        }
        System.out.println(String.format("Schedule: %s done", phase));
    }

    //my task thread
    private static class  Task implements Runnable{
        private String worker;
        private CountDownLatch downLatch;
        private Channel<String> registerChan;
        private DoTaskArgs taskArgs;
        private CopyOnWriteArrayList<Boolean> task_flag;
        Task(String worker, CountDownLatch downLatch, Channel<String> registerChan, CopyOnWriteArrayList<Boolean> task_flag, DoTaskArgs taskArgs){
            this.downLatch = downLatch;
            this.registerChan = registerChan;
            this.worker = worker;
            this.taskArgs = taskArgs;
            this.task_flag = task_flag;
        }

        @Override
        public  void  run(){
            //run task
            Call.getWorkerRpcService(this.worker).doTask(this.taskArgs);
            this.downLatch.countDown();
            //mark the task done
            this.task_flag.set(this.taskArgs.taskNum, true);
            try {
                registerChan.write(this.worker);
            }
            catch (InterruptedException i){
                i.printStackTrace();
            }
        }
    }
}
