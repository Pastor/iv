package ru.iv.support;

import java.util.Set;
import java.util.concurrent.BlockingQueue;

public interface DeviceController {

    Set<DeviceInfo> listDevices();

    void send(Device device, String command);

    boolean hasDevice(Device device);

    BlockingQueue<Event> events();
}
