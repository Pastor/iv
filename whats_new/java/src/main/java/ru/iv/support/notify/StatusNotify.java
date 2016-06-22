package ru.iv.support.notify;

import ru.iv.support.Event;

@SuppressWarnings({"unused", "FieldCanBeLocal"})
final class StatusNotify extends DeviceNotify {
    StatusNotify(Event event) {
        super(event.type == Event.Type.CONNECTED ? Type.DEVICE_CONNECTED : Type.DEVICE_DISCONNECTED, event);
    }
}
