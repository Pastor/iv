package ru.iv.support.entity;

import lombok.Data;
import lombok.EqualsAndHashCode;
import lombok.RequiredArgsConstructor;
import org.hibernate.validator.constraints.NotEmpty;

import javax.persistence.*;

@Entity
@Table(name = "Event")
@Data
@EqualsAndHashCode(callSuper = true)
@RequiredArgsConstructor
public final class Event extends AbstractEntity {
    @NotEmpty
    @Column(name = "name", nullable = false)
    private String name;

    @PrimaryKeyJoinColumn(name = "session_id", referencedColumnName = "id")
    @ManyToOne(fetch = FetchType.LAZY, optional = false, cascade = CascadeType.REMOVE)
    private Session session;
}
