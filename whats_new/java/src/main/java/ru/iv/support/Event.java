package ru.iv.support;

public final class Event {
    private static final Packet[] EMPTY = new Packet[0];
    public final Device device;
    public final Firmware firmware;
    public final Type type;
    public final Packet[] packets;

    public Event(Device device, Firmware firmware, Type type, Packet[] packets) {
        this.device = device;
        this.firmware = firmware;
        this.type = type;
        this.packets = packets;
    }

    public Event(Device device, Firmware firmware, Type type) {
        this(device, firmware, type, EMPTY);
    }

    public Event(Device device, Firmware firmware, Packet[] packets) {
        this(device, firmware, Type.PACKETS, packets);
    }

    public enum Type {
        CONNECTED,
        DISCONNECTED,
        PACKETS
    }
}
