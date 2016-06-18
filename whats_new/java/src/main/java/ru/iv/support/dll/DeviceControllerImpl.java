package ru.iv.support.dll;

import org.springframework.stereotype.Service;
import ru.iv.support.*;

import javax.annotation.PostConstruct;
import java.util.concurrent.BlockingQueue;
import java.util.concurrent.LinkedBlockingQueue;
import java.util.concurrent.TimeUnit;

@Service
final class DeviceControllerImpl implements DeviceController, Callback {
    private final BlockingQueue<Event> events = new LinkedBlockingQueue<>(1000);


    @PostConstruct
    private void init() {
        Library.register(this);
    }

    @Override
    public void send(Device device, String command) {
        Library.send(device, command, true);
    }

    @Override
    public boolean hasDevice(Device device) {
        return Library.hasDevice(device);
    }

    @Override
    public BlockingQueue<Event> events() {
        return events;
    }

    @Override
    public void connected(Device device, Firmware firmware) {
        events.add(new Event(device, firmware, Event.Type.CONNECTED));
    }

    @Override
    public void disconnected(Device device, Firmware firmware) {
        events.add(new Event(device, firmware, Event.Type.DISCONNECTED));
    }

    @Override
    public void handle(Device device, Firmware firmware, Packet[] packets, int count) {
        final Packet[] newPackets = new Packet[count];
        System.arraycopy(packets, 0, newPackets, 0, count);
        events.add(new Event(device, firmware, newPackets));
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
