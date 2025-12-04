import mariadb from 'mariadb';
import dbinfo from './dbinfo.js';
const config = dbinfo.local;

const pool = mariadb.createPool({
      host: config.host,
      port: config.port,
      user: config.user,
      password: config.password,
      database: config.database,
      connectionLimit: 3,
      dateStrings: true,
      bigIntAsNumber: true,
      insertIdAsNumber: true,
      decimalAsNumber: true,
      multipleStatements: true,
      validationQuery: 'select 1',
      testWhileIdle: true

});

export default pool;
