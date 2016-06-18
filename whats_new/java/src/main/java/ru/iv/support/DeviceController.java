package ru.iv.support;

import ru.iv.support.dll.Callback;

import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;
import java.util.concurrent.BlockingQueue;
import java.util.concurrent.LinkedBlockingQueue;
import java.util.concurrent.TimeUnit;

public final class DeviceController implements Callback {
    public final BlockingQueue<Event> events = new LinkedBlockingQueue<>(1000);

    private final Method send;
    private final Method hasDevice;

    private DeviceController() {
        try {
            final Class<?> aClass = Class.forName("ru.iv.support.dll.Library");
            final Method register = aClass.getDeclaredMethod("register", Callback.class);
            send = aClass.getDeclaredMethod("send", Device.class, String.class, boolean.class);
            hasDevice = aClass.getDeclaredMethod("hasDevice", Device.class);
            register.setAccessible(true);
            send.setAccessible(true);
            hasDevice.setAccessible(true);
            register.invoke(null, this);
        } catch (Exception ex) {
            throw new RuntimeException(ex);
        }
    }

    public static DeviceController getDefault() {
        return Holder.INSTANCE;
    }

    public void send(Device device, String command) {
        try {
            send.invoke(null, device, command, true);
        } catch (IllegalAccessException | InvocationTargetException e) {
            e.printStackTrace();
        }
    }

    public boolean hasDevice(Device device) {
        try {
            return (boolean) hasDevice.invoke(null, device);
        } catch (IllegalAccessException | InvocationTargetException e) {
            return false;
        }
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

    private static final class Holder {
        private static final DeviceController INSTANCE = new DeviceController();
    }

    public static void main(String[] args) {
        final BlockingQueue<Event> events = DeviceController.getDefault().events;
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
