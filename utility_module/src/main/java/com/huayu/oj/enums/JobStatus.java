package com.huayu.oj.enums;

/**
 * Created by Administrator on 14-9-24.
 */
public enum JobStatus {

    WAITING(1, "待处理"),
    PROCESSING(2, "处理中"),
    COMPLETE(3, "已完成"),
    TIMEOUT(4, "已超时");


    private int value;
    private String name;

    private JobStatus(int value, String name) {
        this.value = value;
        this.name = name;
    }

    public static String tryParse(int value) {
        for(JobStatus js : JobStatus.values()) {
            if(js.getValue() == value) {
                return js.getName();
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
