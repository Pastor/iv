package ru.iv.support.dll;

import ru.iv.support.Packet;

interface Callback {

    void connected(int device, int firmware);

    void disconnected(int device, int firmware);

    void handle(int device, int firmware, Packet[] packets, int count);
}
