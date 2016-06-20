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
@Table(name = "Answer")
@Data
@EqualsAndHashCode(callSuper = true)
@RequiredArgsConstructor
public final class Answer extends AbstractEntity {
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
    @PrimaryKeyJoinColumn(name = "result_id", referencedColumnName = "id")
    @ManyToOne(fetch = FetchType.LAZY, optional = false, cascade = CascadeType.REMOVE)
    private QuestionResult questionResult;


    public static Answer of(QuestionResult questionResult, Device device, Firmware firmware, Packet packet) {
        final Answer answer = new Answer();
        answer.setQuestionResult(questionResult);
        answer.setDevice(device.ordinal());
        answer.setFirmware(firmware.ordinal());
        answer.setGroup(packet.group);
        answer.setIndex(packet.index);
        answer.setBattery(packet.battery);
        answer.setTimeout(packet.timeout);
        answer.setEnter(packet.enter);
        return answer;
    }
}