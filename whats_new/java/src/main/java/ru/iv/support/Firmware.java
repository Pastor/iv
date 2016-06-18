package ru.iv.support;

public enum Firmware {
    FW45,
    FW60;

    public static Firmware valueOf(int id) {
        for (Firmware d : values()) {
            if (d.ordinal() == id)
                return d;
        }
        throw new IllegalArgumentException();
    }
}
