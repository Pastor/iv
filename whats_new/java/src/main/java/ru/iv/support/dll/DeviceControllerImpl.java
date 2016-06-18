package ru.iv.support.dll;

import lombok.extern.slf4j.Slf4j;
import org.springframework.stereotype.Service;
import ru.iv.support.*;

import javax.annotation.PostConstruct;
import java.util.concurrent.BlockingQueue;
import java.util.concurrent.LinkedBlockingQueue;
import java.util.concurrent.TimeUnit;

@Service
@Slf4j
final class DeviceControllerImpl implements DeviceController, Callback {
    private final BlockingQueue<Event> events = new LinkedBlockingQueue<>(1000);


    @PostConstruct
    private void init() {
        Library.register(this);
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
        events.add(new Event(Device.valueOf(device), Firmware.valueOf(firmware), Event.Type.CONNECTED));
    }

    @Override
    public void disconnected(int device, int firmware) {
        events.add(new Event(Device.valueOf(device), Firmware.valueOf(firmware), Event.Type.DISCONNECTED));
    }

    @Override
    public void handle(int device, int firmware, Packet[] packets, int count) {
        final Packet[] newPackets = new Packet[count];
        System.arraycopy(packets, 0, newPackets, 0, count);
        events.add(new Event(Device.valueOf(device), Firmware.valueOf(firmware), newPackets));
    }

    public static void main(String[] args) {
        final DeviceControllerImpl controller = new DeviceControllerImpl();
        controller.init();
        final BlockingQueue<Event> events = controller.events();
        new Thread(() -> {
            while (true) {
                final Event event = next(events);
                if (event != null) {
                    System.out.println(event);
                }
            }
        }).start();
    }

    private static Event next(BlockingQueue<Event> events) {
        try {
            return events.poll(1000, TimeUnit.MILLISECONDS);
        } catch (Exception ex) {
            return null;
        }
    }
}
