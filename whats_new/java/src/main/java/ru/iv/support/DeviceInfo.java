package ru.iv.support;

import java.util.Objects;

public final class DeviceInfo {
    private Device device;
    private Firmware firmware;

    public DeviceInfo(Device device, Firmware firmware) {
        this.device = device;
        this.firmware = firmware;
    }

    public DeviceInfo() {
    }

    @Override
    public boolean equals(Object o) {
        if (this == o) return true;
        if (o == null || getClass() != o.getClass()) return false;
        DeviceInfo that = (DeviceInfo) o;
        return device == that.device &&
                firmware == that.firmware;
    }

    @Override
    public int hashCode() {
        return Objects.hash(device, firmware);
    }
}
