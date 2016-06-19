package ru.iv.support.dll;

import com.google.common.collect.Sets;
import org.springframework.stereotype.Service;
import ru.iv.support.*;

import javax.annotation.PostConstruct;
import java.util.Set;
import java.util.concurrent.BlockingQueue;
import java.util.concurrent.LinkedBlockingQueue;

@SuppressWarnings("unused")
@Service
final class DeviceControllerImpl implements DeviceController, Callback {
    private final BlockingQueue<Event> events = new LinkedBlockingQueue<>(1000);
    private final Set<DeviceInfo> devices = Sets.newConcurrentHashSet();

    @PostConstruct
    private void init() {
        Library.register(this);
    }

    @Override
    public Set<DeviceInfo> listDevices() {
        return devices;
    }

    @Override
    public void send(Device device, String command) {
        Library.send(device.ordinal(), command, true);
    }

    @Override
    public boolean hasDevice(Device device) {
        return Library.hasDevice(device.ordinal());
    }

    @Override
    public BlockingQueue<Event> events() {
        return events;
    }

    @Override
    public void connected(int device, int firmware) {
        final Device deviceOf = Device.valueOf(device);
        final Firmware firmwareOf = Firmware.valueOf(firmware);
        events.add(new Event(deviceOf, firmwareOf, Event.Type.CONNECTED));
        devices.add(new DeviceInfo(deviceOf, firmwareOf));
    }

    @Override
    public void disconnected(int device, int firmware) {
        final Device deviceOf = Device.valueOf(device);
        final Firmware firmwareOf = Firmware.valueOf(firmware);
        events.add(new Event(deviceOf, firmwareOf, Event.Type.DISCONNECTED));
        devices.remove(new DeviceInfo(deviceOf, firmwareOf));
    }

    @Override
    public void handle(int device, int firmware, Packet[] packets, int count) {
        final Packet[] newPackets = new Packet[count];
        System.arraycopy(packets, 0, newPackets, 0, count);
        events.add(new Event(Device.valueOf(device), Firmware.valueOf(firmware), newPackets));
    }
}
