package com.huayu.oj.judge;

import java.io.IOException;
import java.util.HashMap;
import java.util.LinkedHashMap;
import java.util.Map;

/**
 * @author wangchaoxu.
 */
public class JudgeClient {
    private static final String OJ_READ_PIPE_PREFIX = "oj_read_pipe_";
    private static final String OJ_WRITE_PIPE_PREFIX = "oj_write_pipe_";

    private static Map<String, String> properties;

    public static void main(String[] args) {
        int solutionId = getSolutionId();
        buildProperties(solutionId);

        //根据solutionId创建编译判题进程
        Process judgeProcess = null;
        Runtime run = Runtime.getRuntime();
        try {
            String cmd = "./judge_client -solutionId " + solutionId;
            judgeProcess = run.exec(cmd);
        } catch (IOException e) {
            e.printStackTrace();
        }

        System.out.println(judgeProcess);

        try {
            PipeStream pipeStream = null;
            try {
                while (true) {
                    pipeStream = new PipeStream(OJ_READ_PIPE_PREFIX + solutionId, OJ_WRITE_PIPE_PREFIX + solutionId);
                    String readStr = pipeStream.readString();
                    System.out.print("read data: ");
                    System.out.println(readStr);

                    if (readStr == null || readStr.startsWith("END_PIPE")) {
                        break;
                    }
                    String responseStr = getQueryResponse(readStr);
                    if (responseStr == null) {
                        handleOperateRequest(readStr);
                        responseStr = "no_response_query";
                    }
                    pipeStream.writeString(responseStr);
                }

            } finally {
                if (pipeStream != null) {
                    pipeStream.close();
                }
            }
        } catch (Exception e) {
            e.printStackTrace();
        }


        try {
            Thread.sleep(10000);
        } catch (InterruptedException e) {
            e.printStackTrace();
        }

//        if (judgeProcess != null) {
//            judgeProcess.destroy();
//        }


    }

    /**
     * 处理“操作”类型的请求
     */
    private static void handleOperateRequest(String req) {
        final String SEPARATOR = "|||";
        int indexOfSeparator = req.indexOf(SEPARATOR);
        String operateType = req.substring(0, indexOfSeparator);
        String operateData = req.substring(indexOfSeparator + SEPARATOR.length());
        if (operateType.equals("updateSolution")) {
            handleUpdateSolution(operateData);
        } else if (operateType.equals("addCeInfo")) {
            handleAddCeInfo(operateData);
        } else if (operateType.equals("addReInfo")) {
            handleAddReInfo(operateData);
        } else if (operateType.equals("log")) {
            handleLog(operateData);
        }
    }

    private static void handleLog(String operateData) {
        System.out.println("log: " + operateData);
    }


    /**
     *  获取查询的回复
     * @param req
     * @return
     */
    private static String getQueryResponse(String req) {
        for (String key : properties.keySet()) {
            if (key.equals(req)) {
                return properties.get(key);
            }
        }
        return null;
    }

    /**
     * 处理pipe传来的UpdateSolution请求
     * @param operateData
     */
    private static void handleUpdateSolution(String operateData) {
        String[] pairArray = operateData.split("&");
        for (int i = 0; i < pairArray.length; ++i) {
            System.out.println(pairArray[i]);
        }
    }

    /**
     * 处理pipe传来的addCeInfo请求
     * @param operateData
     */
    private static void handleAddCeInfo(String operateData) {
        System.out.println(operateData);
    }

    /**
     * 处理pipe传来的addReInfo请求
     * @param operateData
     */
    private static void handleAddReInfo(String operateData) {
        System.out.println(operateData);
    }

    /**
     * 根据solutionId获取solution info
     * @param solutionId
     */
    private static void buildProperties(int solutionId) {
        //根据solutionId查询
        properties = new HashMap<String, String>();
        properties.put("problemId", "654123");
        properties.put("userId", "userId85757");
        properties.put("lang", "9");
        properties.put("timeLimit", "300");
        properties.put("memLimit", "366");
        properties.put("spj", "0");
        //properties.put("customInput", "");
        properties.put("sourceCode", "using System;\n" +
                "    class Mainsss\n" +
                "    {\n" +
                "        static void Main(string[] args)\n" +
                "        {\n" +
                "            String input = Console.ReadLine();\n" +
                "            while (input != null) {\n" +
                "                string[] s = input.Split(' ');\n" +
                "                Console.WriteLine((Convert.ToInt32(s[0]) + Convert.ToInt32(s[1])).ToString());\n" +
                "                input = Console.ReadLine();\n" +
                "            }\n" +
                "        }\n" +
                "    }\n");

    }

    //获取需要执行的jobID
    private static Integer getSolutionId() {
        return 123;
    }
}
