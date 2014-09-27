package com.huayu.oj.entry;

import com.huayu.oj.enums.JobStatus;
import com.huayu.oj.enums.JudgeCodeType;

import java.util.UUID;

/**
 * 判定Job的实体类
 */
public class JudgeJob {

    private String jobId;
    private JudgeCodeType codeType;
    private long committerId;
    private String judgeCode;
    private JobStatus jobStatus;
    private long createTime;
    private long lastUpdateTime;

    public JudgeJob() {
        this.jobId = UUID.randomUUID().toString();
    }

    public String getJobId() {
        return jobId;
    }

    public JudgeCodeType getCodeType() {
        return codeType;
    }

    public void setCodeType(JudgeCodeType codeType) {
        this.codeType = codeType;
    }

    public long getCommitterId() {
        return committerId;
    }

    public void setCommitterId(long committerId) {
        this.committerId = committerId;
    }

    public String getJudgeCode() {
        return judgeCode;
    }

    public void setJudgeCode(String judgeCode) {
        this.judgeCode = judgeCode;
    }

    public JobStatus getJobStatus() {
        return jobStatus;
    }

    public void setJobStatus(JobStatus jobStatus) {
        this.jobStatus = jobStatus;
    }

    public long getCreateTime() {
        return createTime;
    }

    public void setCreateTime(long createTime) {
        this.createTime = createTime;
    }

    public long getLastUpdateTime() {
        return lastUpdateTime;
    }

    public void setLastUpdateTime(long lastUpdateTime) {
        this.lastUpdateTime = lastUpdateTime;
    }
}
