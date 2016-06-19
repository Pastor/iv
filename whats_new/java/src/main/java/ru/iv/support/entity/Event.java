package ru.iv.support.entity;

import com.fasterxml.jackson.annotation.JsonIgnore;
import lombok.Data;
import lombok.EqualsAndHashCode;
import lombok.RequiredArgsConstructor;
import org.hibernate.validator.constraints.NotEmpty;
import ru.iv.support.Device;
import ru.iv.support.Firmware;
import ru.iv.support.Packet;

import javax.persistence.*;

@Entity
@Table(name = "Event")
@Data
@EqualsAndHashCode(callSuper = true)
@RequiredArgsConstructor
public final class Event extends AbstractEntity {
    @Column(name = "device", nullable = false)
    private Integer device;
    @Column(name = "firmware", nullable = false)
    private Integer firmware;
    @Column(name = "device_group", nullable = false)
    private Integer group;
    @Column(name = "device_index", nullable = false)
    private Integer index;
    @Column(name = "battery", nullable = false)
    private Integer battery;
    @Column(name = "timeout", nullable = false)
    private Integer timeout;

    @NotEmpty
    @Column(name = "enter", nullable = false)
    private String enter;

    @JsonIgnore
    @PrimaryKeyJoinColumn(name = "session_id", referencedColumnName = "id")
    @ManyToOne(fetch = FetchType.LAZY, optional = false, cascade = CascadeType.REMOVE)
    private Session session;


    public static Event of(Session session, Device device, Firmware firmware, Packet packet) {
        final Event event = new Event();
        event.setSession(session);
        event.setDevice(device.ordinal());
        event.setFirmware(firmware.ordinal());
        event.setGroup(packet.group);
        event.setIndex(packet.index);
        event.setBattery(packet.battery);
        event.setTimeout(packet.timeout);
        event.setEnter(packet.enter);
        return event;
    }
}