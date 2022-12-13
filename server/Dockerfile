FROM node:18.4-alpine
WORKDIR /app

COPY ./bin /app/bin
COPY ./models /app/models
COPY ./public /app/public
COPY ./routes /app/routes
COPY ./services /app/services
COPY ./views /app/views
COPY ./app.ts /app/app.ts
COPY ./package.json /app/package.json
COPY ./package-lock.json /app/package-lock.json
COPY ./tsconfig.json /app/tsconfig.json

RUN apk update
RUN apk add wget
RUN npm install

CMD ["npm", "run", "prod", "--prefix", "/app"]
