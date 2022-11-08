import createError from 'http-errors';
import express from 'express';
import path from 'path';
import logger from 'morgan';
import rateLimit from 'express-rate-limit';

import indexRouter from './routes/index.router';

var app = express();

// View engine setup
app.set('views', path.join(__dirname, 'views'));
app.set('view engine', 'pug');

// Reverse proxy
app.set('trust proxy', '192.168.1.0/24')

// Middlewares
app.use(logger('dev'));
app.use(express.json());
app.use(express.urlencoded({ extended: false }));
app.use(express.static(path.join(__dirname, 'public')));

// Rate limiter
const rateLimiter = rateLimit({
  windowMs: parseInt(process.env.WINDOW_SIZE_MS ?? "900000"),
  max: parseInt(process.env.MAX_CONNECTIONS_PER_WINDOW ?? "10"),
  standardHeaders: true, // Return rate limit info in the `RateLimit-*` headers
  legacyHeaders: false, // Disable the `X-RateLimit-*` headers
})

// Routes
app.use(rateLimiter);
app.use('/', indexRouter);


// catch 404 and forward to error handler
app.use(function(req, res, next) {
  next(createError(404));
});

// error handler
app.use(function(err:any, req: Request | any, res: Response | any, next:any) {
  // set locals, only providing error in development
  res.locals.message = err.message;
  res.locals.error = req.app.get('env') === 'development' ? err : {};

  // render the error page
  res.status(err.status || 500);
  res.render('error');
});

export default app;
