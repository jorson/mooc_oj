package com.huayu.oj.enums;

/**
 * Created by Administrator on 14-9-24.
 */
public enum JudgeCodeType {
    C(1, "Stander C"),
    C_PLUS_PLUS(2, "C++"),
    JAVA(3, "Java"),
    C_SHARP(4, "C#"),
    JAVA_SCRIPT(5, "javascript");

    private int value;
    private String name;

    private JudgeCodeType(int value, String name) {
        this.value = value;
        this.name = name;
    }

    public static String tryParse(int value) {
        for(JudgeCodeType jc : JudgeCodeType.values()) {
            if(jc.getValue() == value) {
                return jc.getName();
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
