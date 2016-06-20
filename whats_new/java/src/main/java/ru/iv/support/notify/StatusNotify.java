package ru.iv.support.notify;

import ru.iv.support.Event;

@SuppressWarnings({"unused", "FieldCanBeLocal"})
final class StatusNotify extends DeviceNotify {
    private final Event.Type type;

    StatusNotify(Event event) {
        super(event);
        this.type = event.type;
    }
}
