package ru.iv.support.rest;

import com.fasterxml.jackson.databind.DeserializationFeature;
import com.fasterxml.jackson.databind.ObjectMapper;
import com.fasterxml.jackson.databind.SerializationFeature;
import lombok.extern.slf4j.Slf4j;
import org.h2.Driver;
import org.hibernate.jpa.HibernatePersistenceProvider;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.boot.SpringApplication;
import org.springframework.boot.autoconfigure.EnableAutoConfiguration;
import org.springframework.boot.autoconfigure.SpringBootApplication;
import org.springframework.boot.context.properties.EnableConfigurationProperties;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.ComponentScan;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Primary;
import org.springframework.core.task.SimpleAsyncTaskExecutor;
import org.springframework.core.task.TaskExecutor;
import org.springframework.data.jpa.repository.config.EnableJpaRepositories;
import org.springframework.http.converter.json.Jackson2ObjectMapperBuilder;
import org.springframework.http.server.ServerHttpRequest;
import org.springframework.http.server.ServerHttpResponse;
import org.springframework.jdbc.datasource.DriverManagerDataSource;
import org.springframework.orm.jpa.JpaTransactionManager;
import org.springframework.orm.jpa.LocalContainerEntityManagerFactoryBean;
import org.springframework.orm.jpa.vendor.HibernateJpaVendorAdapter;
import org.springframework.scheduling.annotation.EnableAsync;
import org.springframework.transaction.annotation.EnableTransactionManagement;
import org.springframework.web.servlet.config.annotation.EnableWebMvc;
import org.springframework.web.socket.WebSocketExtension;
import org.springframework.web.socket.WebSocketHandler;
import org.springframework.web.socket.config.annotation.EnableWebSocket;
import org.springframework.web.socket.config.annotation.WebSocketConfigurer;
import org.springframework.web.socket.config.annotation.WebSocketHandlerRegistry;
import org.springframework.web.socket.server.HandshakeFailureException;
import org.springframework.web.socket.server.RequestUpgradeStrategy;
import org.springframework.web.socket.server.standard.TomcatRequestUpgradeStrategy;
import org.springframework.web.socket.server.support.DefaultHandshakeHandler;
import ru.iv.support.notify.WebNotifyController;

import javax.sql.DataSource;
import java.security.Principal;
import java.sql.SQLException;
import java.time.format.DateTimeFormatter;
import java.util.List;
import java.util.Map;
import java.util.Properties;

@EnableAutoConfiguration
@EnableWebSocket
@EnableWebMvc
@SpringBootApplication
@EnableAsync
@EnableConfigurationProperties
@EnableTransactionManagement
@EnableJpaRepositories(
        basePackages = {"ru.iv.support.repository"},
        basePackageClasses = {},
        entityManagerFactoryRef = "emFactory",
        transactionManagerRef = "emTransactionManager")
@Slf4j
public class Application implements WebSocketConfigurer {
    private static final DateTimeFormatter formatter = DateTimeFormatter.ISO_LOCAL_DATE;

    @Autowired
    private WebNotifyController webController;

    @ComponentScan({
            "ru.iv.support.rest",
            "ru.iv.support.service",
            "ru.iv.support.dll",
            "ru.iv.support.repository",
    })
    @Configuration
    public static class SupportConfiguration {

    }


    @Bean
    @Primary
    public DataSource internalDataSource() {
        final DriverManagerDataSource dataSource = new DriverManagerDataSource();
        dataSource.setDriverClassName(Driver.class.getName());
        dataSource.setUsername("sa");
        final String url = "jdbc:h2:file:~/" + "support"/*formatter.format(LocalDate.now())*/;
        log.debug("Url: {}", url);
        dataSource.setUrl(url);
        return dataSource;
    }

    @Bean
    @Primary
    public ObjectMapper objectMapper(Jackson2ObjectMapperBuilder builder) {
        final ObjectMapper objectMapper = builder.createXmlMapper(false).build();
        objectMapper.configure(SerializationFeature.WRITE_DATES_AS_TIMESTAMPS, false);
        objectMapper.configure(SerializationFeature.WRITE_DATE_TIMESTAMPS_AS_NANOSECONDS, false);
        objectMapper.configure(SerializationFeature.FAIL_ON_EMPTY_BEANS, false);
        objectMapper.configure(DeserializationFeature.READ_DATE_TIMESTAMPS_AS_NANOSECONDS, false);
        return objectMapper;
    }

    @Bean(name = "emFactory")
    public LocalContainerEntityManagerFactoryBean entityManagerFactory() {
        final LocalContainerEntityManagerFactoryBean emFactory = new LocalContainerEntityManagerFactoryBean();
        emFactory.setDataSource(internalDataSource());
        emFactory.setPersistenceProviderClass(HibernatePersistenceProvider.class);
        final HibernateJpaVendorAdapter vendorAdapter = new HibernateJpaVendorAdapter();
        vendorAdapter.setGenerateDdl(true);
        emFactory.setJpaVendorAdapter(vendorAdapter);
        final Properties properties = new Properties();
        properties.setProperty("hibernate.dialect", "org.hibernate.dialect.H2Dialect");
        properties.setProperty("hibernate.hbm2ddl.auto", "update");
        properties.setProperty("hibernate.show_sql", "true");
        emFactory.setJpaProperties(properties);
        emFactory.setPackagesToScan("ru.iv.support.entity");
        return emFactory;
    }

    @Bean(name = "emTransactionManager")
    @Primary
    public JpaTransactionManager transactionManager() {
        final JpaTransactionManager transactionManager = new JpaTransactionManager();
        transactionManager.setEntityManagerFactory(entityManagerFactory().getObject());
        return transactionManager;
    }

    @Bean(name = "questionTaskExecutor")
    public TaskExecutor getQuestionTaskExecutor() {
        return new SimpleAsyncTaskExecutor();
    }

    @Bean(name = "deviceTaskExecutor")
    public TaskExecutor getDeviceTaskExecutor() {
        return new SimpleAsyncTaskExecutor();
    }

    public static void main(String[] args) throws SQLException {
        SpringApplication.run(Application.class, args);
    }

    @Override
    public void registerWebSocketHandlers(WebSocketHandlerRegistry registry) {
        registry.addHandler(webController, "/notification").setHandshakeHandler(new DefaultHandshakeHandler(new RequestUpgradeStrategy() {
            private final RequestUpgradeStrategy strategy = new TomcatRequestUpgradeStrategy();

            @Override
            public String[] getSupportedVersions() {
                return strategy.getSupportedVersions();
            }

            @Override
            public List<WebSocketExtension> getSupportedExtensions(ServerHttpRequest request) {
                return strategy.getSupportedExtensions(request);
            }

            @Override
            public void upgrade(ServerHttpRequest request,
                                ServerHttpResponse response,
                                String selectedProtocol,
                                List<WebSocketExtension> selectedExtensions,
                                Principal user,
                                WebSocketHandler wsHandler,
                                Map<String, Object> attributes) throws HandshakeFailureException {
                strategy.upgrade(request, response, selectedProtocol, selectedExtensions, user, wsHandler, attributes);
            }
        }));
    }
}
