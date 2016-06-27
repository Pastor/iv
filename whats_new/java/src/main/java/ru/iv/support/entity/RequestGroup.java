package ru.iv.support.entity;

import com.fasterxml.jackson.annotation.JsonIgnore;
import lombok.Data;
import lombok.EqualsAndHashCode;
import lombok.RequiredArgsConstructor;
import ru.iv.support.Device;

import javax.persistence.*;
import javax.validation.constraints.NotNull;

@Entity
@Table(name = "RequestGroup")
@Data
@EqualsAndHashCode(callSuper = true)
@RequiredArgsConstructor
public final class RequestGroup extends AbstractEntity {
    @NotNull
    @Column(name = "group_number", nullable = false)
    private Integer group;

    @NotNull
    @Column(name = "firmware", nullable = false)
    private Integer firmware;

    @NotNull
    @Column(name = "device", nullable = false)
    private Integer device;

    @JsonIgnore
    @NotNull
    @PrimaryKeyJoinColumn(name = "question_sequence_id", referencedColumnName = "id")
    @ManyToOne(fetch = FetchType.LAZY, cascade = CascadeType.REMOVE)
    private QuestionSequence sequence;

    public String toDeviceQuery() {
        return "Q" + Integer.toHexString(group).toUpperCase();
    }

    public Device toDevice() {
        return Device.valueOf(device);
    }
}
