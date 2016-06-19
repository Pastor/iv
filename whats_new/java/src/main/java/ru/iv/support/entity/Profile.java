package ru.iv.support.entity;

import com.fasterxml.jackson.annotation.JsonIgnore;
import lombok.*;
import org.hibernate.validator.constraints.NotEmpty;

import javax.persistence.*;
import java.util.Set;

@Entity
@Table(name = "Profile")
@Data
@EqualsAndHashCode(callSuper = true, exclude = {"sessions"})
@RequiredArgsConstructor
public final class Profile extends AbstractEntity {
    @NotEmpty
    @Column(name = "name", nullable = false)
    private String name;


    @JsonIgnore
    @Setter(AccessLevel.NONE)
    @OneToMany(fetch = FetchType.LAZY, mappedBy = "profile", cascade = CascadeType.ALL, orphanRemoval = true)
    @OrderBy("id")
    private Set<Session> sessions;
}
