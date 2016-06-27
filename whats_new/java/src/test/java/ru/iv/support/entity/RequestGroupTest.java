package ru.iv.support.entity;

import org.junit.Test;

import static org.junit.Assert.*;

public final class RequestGroupTest {
    @Test
    public void toDeviceQuery() throws Exception {
        final int groupIndex = Integer.parseInt("2D", 16);
        final RequestGroup group = new RequestGroup();
        group.setGroup(groupIndex);
        assertEquals("Q2D", group.toDeviceQuery());
    }
}