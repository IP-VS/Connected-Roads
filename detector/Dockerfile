FROM docker.io/python:3.11-slim

WORKDIR /detector

RUN pip install --no-cache-dir pipenv

COPY Pipfile Pipfile.lock ./
RUN pipenv install --quiet --clear

COPY detector.py ./

CMD ["sh", "-c", "pipenv run python detector.py --port ${LISTEN_PORT}"]
