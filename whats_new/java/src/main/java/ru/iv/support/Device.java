package ru.iv.support;

public enum Device {
    HID,
    FTDI;

    public static Device valueOf(int id) {
        for (Device d: values()) {
            if (d.ordinal() == id)
                return d;
        }
        throw new IllegalArgumentException();
    }
}
