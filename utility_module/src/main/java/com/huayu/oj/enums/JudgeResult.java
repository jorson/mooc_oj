package com.huayu.oj.enums;

/**
 * Created by Jorson on 14-9-25.
 */
public enum JudgeResult {

    CORRECT(1, "结果正确"),
    ERROR(2, "结果错误"),
    FORMAT_ERROR(3, "格式错误"),
    RUN_TIMEOUT(4, "执行超时"),
    OUT_OF_MEMORY(5, "内存超出"),
    OUTPUT_LIMIT(6, "输出超出"),
    RUNTIME_ERROR(7, "运行时错误"),
    COMPILE_ERROR(8, "编译错误");

    private int value;
    private String name;

    private JudgeResult(int value, String name) {
        this.value = value;
        this.name = name;
    }

    public static String tryParse(int value) {
        for(JudgeResult jr : JudgeResult.values()) {
            if(jr.getValue() == value) {
                return jr.getName();
            }
        }
        return null;
    }

    public int getValue() {
        return value;
    }

    public String getName() {
        return name;
    }
}
