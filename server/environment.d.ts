declare global {
    namespace NodeJS {
        interface ProcessEnv {
            NODE_ENV: 'development' | 'production';
            WINDOW_SIZE_MS: string;
            MAX_CONNECTIONS_PER_WINDOW: string;
            PORT: string;
            SERIAL_PORT: string;
            BAUD_RATE: string;
        }
    }
}

export { }