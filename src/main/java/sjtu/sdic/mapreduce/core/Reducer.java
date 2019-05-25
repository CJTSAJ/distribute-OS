package sjtu.sdic.mapreduce.core;

import com.alibaba.fastjson.JSONArray;
import com.alibaba.fastjson.JSONObject;
import sjtu.sdic.mapreduce.common.KeyValue;
import sjtu.sdic.mapreduce.common.Utils;

import java.io.FileOutputStream;
import java.io.File;
import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Paths;
import java.util.*;

/**
 * Created by Cachhe on 2019/4/19.
 */
public class Reducer {

    /**
     * 
     * 	doReduce manages one reduce task: it should read the intermediate
     * 	files for the task, sort the intermediate key/value pairs by key,
     * 	call the user-defined reduce function {@code reduceFunc} for each key,
     * 	and write reduceFunc's output to disk.
     * 	
     * 	You'll need to read one intermediate file from each map task;
     * 	{@code reduceName(jobName, m, reduceTask)} yields the file
     * 	name from map task m.
     *
     * 	Your {@code doMap()} encoded the key/value pairs in the intermediate
     * 	files, so you will need to decode them. If you used JSON, you can refer
     * 	to related docs to know how to decode.
     * 	
     *  In the original paper, sorting is optional but helpful. Here you are
     *  also required to do sorting. Lib is allowed.
     * 	
     * 	{@code reduceFunc()} is the application's reduce function. You should
     * 	call it once per distinct key, with a slice of all the values
     * 	for that key. {@code reduceFunc()} returns the reduced value for that
     * 	key.
     * 	
     * 	You should write the reduce output as JSON encoded KeyValue
     * 	objects to the file named outFile. We require you to use JSON
     * 	because that is what the merger than combines the output
     * 	from all the reduce tasks expects. There is nothing special about
     * 	JSON -- it is just the marshalling format we chose to use.
     * 	
     * 	Your code here (Part I).
     * 	
     * 	
     * @param jobName the name of the whole MapReduce job
     * @param reduceTask which reduce task this is
     * @param outFile write the output here
     * @param nMap the number of map tasks that were run ("M" in the paper)
     * @param reduceFunc user-defined reduce function
     */
    public static void doReduce(String jobName, int reduceTask, String outFile, int nMap, ReduceFunc reduceFunc) {
        //read all domap file for the task and put them in a list
        List<KeyValue> task_contents = new ArrayList<>();
        for(int i = 0; i < nMap; i++){
            //get the file name, and read the content
            String file_name = Utils.reduceName(jobName, i, reduceTask);
            String tmp_str = "";
            try{
                tmp_str = new String(Files.readAllBytes(Paths.get(file_name)));
            }
            catch (IOException e){
                e.printStackTrace();
            }

            List<KeyValue> part_list = JSONArray.parseArray(tmp_str, KeyValue.class);
            task_contents.addAll(part_list);
        }

        //sorting by key is helpful
        Collections.sort(task_contents, new Comparator<KeyValue>() {
            @Override
            public int compare(KeyValue o1, KeyValue o2) {
                return o1.key.compareTo(o2.key);
            }
        });

        //merge values that have the same key
        Map<String, List<String>> merge_content = new HashMap<>();
        for(int i = 0; i < task_contents.size(); i++){
            if(merge_content.get(task_contents.get(i).key) == null){
                List<String> tmp_list = new ArrayList<>();
                tmp_list.add(task_contents.get(i).value);
                merge_content.put(task_contents.get(i).key, tmp_list);
            }
            else{
                List<String> tmp_list = merge_content.get(task_contents.get(i).key);
                tmp_list.add(task_contents.get(i).value);
            }
        }

        //call reduceFunc
        JSONObject result = new JSONObject();
        for(String tmp_key : merge_content.keySet()){
            List<String> tmp_list = merge_content.get(tmp_key);
            String[] string_arr = tmp_list.toArray(new String[tmp_list.size()]);

            String reduce_str = reduceFunc.reduce(tmp_key, string_arr);
            result.put(tmp_key, reduce_str);
        }

        //write json result to file
        File file = new File(outFile);
        FileOutputStream outStr = null;
        try {
            outStr = new FileOutputStream(file);
            String tmp_str = result.toJSONString();
            outStr.write(tmp_str.getBytes());
            outStr.close();
        }
        catch (IOException e){
            e.printStackTrace();
        }
    }
}
