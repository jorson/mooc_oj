package com.huayu.oj.entry;

import com.huayu.oj.enums.JudgeResult;

/**
 * Created by Jorson on 14-9-26.
 */
public class JobResult {

    private int resultId;
    private String judgeJobId;
    private JudgeResult judgeResult;
    private long completeTime;

    public JobResult() {

    }

    public int getResultId() {
        return resultId;
    }

    public void setResultId(int resultId) {
        this.resultId = resultId;
    }

    public String getJudgeJobId() {
        return judgeJobId;
    }

    public void setJudgeJobId(String judgeJobId) {
        this.judgeJobId = judgeJobId;
    }

    public JudgeResult getJudgeResult() {
        return judgeResult;
    }

    public void setJudgeResult(JudgeResult judgeResult) {
        this.judgeResult = judgeResult;
    }

    public long getCompleteTime() {
        return completeTime;
    }

    public void setCompleteTime(long completeTime) {
        this.completeTime = completeTime;
    }
}
