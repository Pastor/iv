package ru.iv.support.dll;

import ru.iv.support.Packet;

public interface Callback {

    void connected(int device, int firmware);

    void disconnected(int device, int firmware);

    void handle(int device, int firmware, Packet[] packets, int count);
}
