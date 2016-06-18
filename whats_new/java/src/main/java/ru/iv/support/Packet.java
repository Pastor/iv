package ru.iv.support;

public final class Packet {
    public int group = 0;
    public int index = 0;
    public int battery = 0;
    public int timeout = 0;
    public String enter = "";

    private Packet() {
    }

    @Override
    public String toString() {
        return "Packet{" +
                "group=" + group +
                ", index=" + index +
                ", battery=" + battery +
                ", timeout=" + timeout +
                ", enter='" + enter + '\'' +
                '}';
    }
}
