package sjtu.sdic.mapreduce;

import org.apache.commons.io.filefilter.WildcardFileFilter;
import sjtu.sdic.mapreduce.common.KeyValue;
import sjtu.sdic.mapreduce.core.Master;
import sjtu.sdic.mapreduce.core.Worker;

import java.io.File;
import java.util.*;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

/**
 * Created by Cachhe on 2019/4/24.
 */
public class InvertedIndex {
    //key: word    value: file that contains this word;
    public static List<KeyValue> mapFunc(String file, String value) {
        Pattern p = Pattern.compile("[a-zA-Z0-9]+");
        Matcher m = p.matcher(value);

        List<KeyValue> ret = new ArrayList<>();
        while(m.find()){
            String tmp_key = m.group();
            ret.add(new KeyValue(tmp_key, file));
        }
        return ret;
    }

    //output: (number of file) file1,file2,file3 ....
    public static String reduceFunc(String key, String[] values) {
        //attention: should delete the repeated file
        Set<String> set = new HashSet();
        for(String tmp_str : values){
            set.add(tmp_str);
        }

        Object[] new_values = set.toArray();
        //the most effective add string: StringBuilder
        StringBuilder sb = new StringBuilder();
        sb.append(String.valueOf(new_values.length));
        sb.append(" ");
        for(Object tmp_str : new_values){
            sb.append(tmp_str);
            sb.append(",");
        }

        sb.deleteCharAt(sb.length() - 1);
        return sb.toString();
    }

    public static void main(String[] args) {
        if (args.length < 3) {
            System.out.println("error: see usage comments in file");
        } else if (args[0].equals("master")) {
            Master mr;

            String src = args[2];
            File file = new File(".");
            String[] files = file.list(new WildcardFileFilter(src));
            if (args[1].equals("sequential")) {
                mr = Master.sequential("iiseq", files, 3, InvertedIndex::mapFunc, InvertedIndex::reduceFunc);
            } else {
                mr = Master.distributed("iiseq", files, 3, args[1]);
            }
            mr.mWait();
        } else {
            Worker.runWorker(args[1], args[2], InvertedIndex::mapFunc, InvertedIndex::reduceFunc, 100, null);
        }
    }
}
