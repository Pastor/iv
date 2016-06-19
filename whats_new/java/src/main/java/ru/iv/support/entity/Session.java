package ru.iv.support.entity;

import lombok.Data;
import lombok.EqualsAndHashCode;
import lombok.RequiredArgsConstructor;

import javax.persistence.*;
import javax.validation.constraints.NotNull;
import javax.validation.constraints.Null;

@Entity
@Table(name = "Session")
@Data
@EqualsAndHashCode(callSuper = true)
@RequiredArgsConstructor
public final class Session extends AbstractEntity {
    public static final String DEFAULT_NAME = "default";

    @NotNull
    @PrimaryKeyJoinColumn(name = "profile_id", referencedColumnName = "id")
    @ManyToOne(fetch = FetchType.LAZY, cascade = CascadeType.REMOVE)
    private Profile profile;

    @Column(name = "name")
    private String name;

    @Column(name = "active", nullable = false)
    private boolean activate;
}
