package ru.iv.support;

import java.util.concurrent.BlockingQueue;

public interface DeviceController {

    void send(Device device, String command);

    boolean hasDevice(Device device);

    BlockingQueue<Event> events();
}
