package com.huayu.oj.repository;

import java.util.Collection;
import java.util.List;

/**
 * Created by Jorson on 14-9-26.
 */
public interface SimpleRepository {

    <TEntry> void create(TEntry entry);

    <TEntry> void update(TEntry entry);

    void delete(int id);

    <TEntry> TEntry get(int id);

    <TEntry> List<TEntry> getList(Collection<Integer> ids);
}
