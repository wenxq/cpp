__author__ = 'xingquan.wxq'

import time, unittest
from sqlite3 import dbapi2 as sqlite

class Database(unittest.TestCase):
    def __init__(self, autocommit=False):
        unittest.TestCase.__init__(self)
        self.con = None
        self.tables = []
        self.schemas = {}
        self.auto_commit = autocommit

    def open(self):
        if self.con is not None:
            self.con.close()
        self.con = self._connect()
        self.cur = self.con.cursor()

    def close(self):
        if self.con is not None:
            self.cur.close()
            self.con.close()

    def autocommit(self, flag=True):
        self.auto_commit = flag

    def commit(self):
        self.con.commit()

    def rollback(self):
        self.con.rollback()

    def show_tables(self):
        raise NotImplementedError('show_tables')

    def desc(self, tablename):
        raise NotImplementedError('desc')

    def _connect(self):
        raise NotImplementedError('_connect')

    def trans_schema(self, fields, pri_key, not_null):
        raise NotImplementedError('trans_schema')

    def _parse(self, info):
        raise NotImplementedError('_parse')

    def _schema(self, tablename):
        raise NotImplementedError('_schema')

    def _get_schema(self, info):
        if type(info) in (str, unicode):
            return info
        fields = info['fields']
        pri_key, not_null = [], []
        if 'pri_key' in info:
            pri_key = info['pri_key']
        if 'not_null' in info:
            not_null = info['not_null']
        return self.trans_schema(fields, pri_key, not_null)

    def create_table(self, tablename, schema):
        self._execute('CREATE TABLE `%s` (%s)' % (tablename, self._get_schema(schema)))

    def drop_table(self, tablename):
        self._execute('DROP TABLE IF EXISTS `%s`' % tablename)

    def _equal_schema(self, left, right):
        self.assertEqual(str(left['pri_key']).lower(), str(right['pri_key']).lower())
        self.assertEqual(str(left['not_null']).lower(), str(right['not_null']).lower())
        for i, fd in enumerate(left['fields']):
            fd2 = right['fields'][i]
            self.assertEqual(fd[0].lower(), fd2[0].lower())
            if fd[1].lower() == 'int' and fd2[1].lower() == 'double':
                continue
            else:
                self.assertEqual(fd[1].lower(), fd2[1].lower())

    def bind_table(self, tablename, schema=None):
        if tablename not in self.show_tables():
            self.assertIsNotNone(schema)
            self.create_table(tablename, schema)

        if schema is not None:
            if type(schema) in (str, unicode):
                self._equal_schema(self._parse(schema), self._schema(tablename))
            else:
                self._equal_schema(schema, self._schema(tablename))

        schema = self._schema(tablename)
        fields = [ info[0] for info in schema['fields'] ]
        self.schemas[tablename] = fields
        self.tables.append(tablename)

    def _get_record(self, values):
        value_list = []
        for v in values:
            if v is None:
                continue
            if type(v) in (str, unicode):
                value_list.append('\'%s\'' % v.replace('\'', '\'\''))
            else:
                value_list.append('%s' % v)
        return value_list

    def _execute(self, sql):
        #print sql
        for i in xrange(5):
            try:
                self.cur.execute(sql)
                break
            except sqlite.OperationalError, e:
                if 'database is locked' in e.message:
                    time.sleep(3)
                    continue
                raise

    def _get_table(self, kwargs):
        table = None
        if kwargs.has_key('__table__'):
            table = kwargs['__table__']
            del kwargs['__table__']
        elif len(self.tables) == 1:
            table = self.tables[0]
        assert table
        return table

    def _get_field(self, fields):
        fds = []
        for fd in fields:
            if fd[-1] != '`' and fd[0] != '`':
                fds.append('`%s`' % fd)
            else:
                fds.append(fd)
        return ','.join(fds)

    def insert_record(self, *args, **kwargs):
        table = self._get_table(kwargs)
        fields = self.schemas[table]
        if len(args) != 0:
            sql = 'INSERT INTO `%s`(%s) VALUES(%s)' \
                  % (table, self._get_field(fields),
                     ', '.join(self._get_record(args)))
        else:
            self.assertNotEqual(0, len(kwargs))
            field = self._get_field([k for k, v in kwargs.items() if v and k])
            values = ', '.join(self._get_record(kwargs.values()))
            sql = 'INSERT INTO `%s`(%s) VALUES(%s)' % (table, field, values)
        self._execute(sql)
        if self.auto_commit: self.commit()

    def insert_records(self, records, **kwargs):
        table = self._get_table(kwargs)
        fields = self.schemas[table]
        sql = 'INSERT INTO `%s` VALUES (%s)' \
              % (table, ', '.join([ '?' for fd in fields ]))
        self.cur.executemany(sql, records)
        if self.auto_commit: self.commit()

    def update_record(self, condition, **kwargs):
        table = self._get_table(kwargs)
        sql = 'UPDATE `%s` SET %s %s' \
              % (table, ', '.join(['`%s`=%s' % (k, v) for k, v in \
                        zip(kwargs.keys(), self._get_record(kwargs.values()))]),
                 condition)
        self._execute(sql)
        if self.auto_commit: self.commit()

    def delete_record(self, condition='', **kwargs):
        table = self._get_table(kwargs)
        sql = 'DELETE FROM `%s` %s' % (table, condition)
        self._execute(sql)
        if self.auto_commit: self.commit()

    def select(self, fields, condition='', **kwargs):
        table = self._get_table(kwargs)
        if fields != '*':
            fields = ', '.join(fields)
        sql = 'SELECT %s FROM `%s` %s' % (fields, table, condition)
        self._execute(sql)
        return self.cur.fetchall()

    def runTest(self):
        pass

class Sqlite(Database):
    def __init__(self, database, autocommit=False):
        self.database = database
        Database.__init__(self, autocommit)

    def _connect(self):
        return sqlite.connect(self.database)

    def select(self, fields, condition='', **kwargs):
        if fields == '*' or fields[0] == '*':
            table, fields = self._get_table(kwargs), []
            for row in self.desc(table):
                fields.append(row[1])
            kwargs['__table__'] = table
        selection = Database.select(self, fields, condition, **kwargs)
        result = []
        for section in selection:
            fds = {}
            for i, fd in enumerate(section):
                fds[fields[i]] = fd
            result.append(fds)
        return result

    def trans_schema(self, fields, pri_key, not_null):
        schema = ''
        for fd in fields:
            line = '%s %s' % (fd[0], fd[1])
            if pri_key and fd[0] in pri_key:
                line += ' PRIMARY KEY'
            if not_null and fd[0] in not_null:
                line += ' NOT NULL'
            if schema:
                schema += ',\n'
            schema += line
        #print schema
        return schema

    def show_tables(self):
        sql = 'SELECT tbl_name FROM sqlite_master WHERE type = \'table\' ORDER BY name'
        self.cur.execute(sql)
        tables = []
        for row in self.cur.fetchall():
            for table in row:
                tables.append(table)
        return tables

    def desc(self, tablename):
        sql = 'PRAGMA table_info(`%s`)' % tablename
        self.cur.execute(sql)
        return self.cur.fetchall()

    def _parse(self, info):
        #print info
        info = info.encode('utf-8')
        schema = { 'fields':[], 'pri_key':[], 'not_null':[] }
        lines = info.split(',')
        for line in lines:
            keys = line.split()
            self.assertGreaterEqual(len(keys), 2)
            schema['fields'].append((keys[0], keys[1]))
            if 'PRIMARY KEY' in line:
                schema['pri_key'].append(keys[0])
                if keys[0] not in schema['not_null']:
                    schema['not_null'].append(keys[0])
            if 'NOT NULL' in line and keys[0] not in schema['not_null']:
                schema['not_null'].append(keys[0])
        return schema

    def _schema(self, tablename):
        sql = 'SELECT sql FROM sqlite_master WHERE type = \'table\' AND tbl_name = \'%s\'' % tablename
        self.cur.execute(sql)
        sql = self.cur.fetchall()[0][0]
        beg = sql.find('(')
        end = sql.rfind(')')
        return self._parse(sql[beg+1:end])

class MySQL(Database):
    def __init__(self, *args, **kwargs):
        if not kwargs.has_key('autocommit'):
            kwargs['autocommit'] = False
        Database.__init__(self, kwargs['autocommit'])
        self.args = args
        self.kwargs = kwargs
        del self.kwargs['autocommit']

    def _connect(self):
        import pymysql
        from pymysql.cursors import DictCursor
        self.kwargs['cursorclass'] = DictCursor
        return pymysql.connect(*self.args, **self.kwargs)

    def trans_schema(self, fields, pri_key, not_null):
        schema = ''
        for fd in fields:
            line = '%s %s' % (fd[0], fd[1])
            if not_null and fd[0] in not_null:
                line += ' NOT NULL'
            if schema:
                schema += ',\n'
            schema += line

        if pri_key:
            schema += ',\nPRIMARY KEY (%s)' % (','.join(pri_key))
        return schema

    def show_tables(self):
        sql = 'SHOW TABLES'
        self.cur.execute(sql)
        tables = []
        for row in self.cur.fetchall():
            for _, table in row.items():
                tables.append(table)
        return tables

    def desc(self, tablename):
        sql = 'DESCRIBE `%s`' % tablename
        self.cur.execute(sql)
        return self.cur.fetchall()

    def _parse(self, info):
        info = info.encode('utf-8')
        schema = { 'fields':[], 'pri_key':[], 'not_null':[] }
        lines = info.split(',')
        for line in lines:
            if 'PRIMARY KEY' in line:
                beg = line.find('(')
                end = line.find(')')
                for name in line[beg+1:end].split(','):
                    schema['pri_key'].append(name.strip())
                    if name not in schema['not_null']:
                        schema['not_null'].append(name)
                continue
            else:
                keys = line.split()
                self.assertGreaterEqual(len(keys), 2)
                if keys[1].lower() == 'int(11)'.encode('utf-8'):
                    keys[1] = 'INT'.encode('utf-8')
                schema['fields'].append((keys[0], keys[1]))
                if 'NOT NULL' in line and keys[0] not in schema['not_null']:
                    schema['not_null'].append(keys[0])
        return schema

    def _schema(self, tablename):
        sql = 'SHOW CREATE TABLE `%s`' % tablename
        self.cur.execute(sql)
        sql = self.cur.fetchall()[0]['Create Table']
        sql = sql.replace('`', '')
        beg = sql.find('(')
        end = sql.rfind(')')
        return self._parse(sql[beg+1:end])

def new_database(dtype, *args, **kwargs):
    dtype = dtype.lower()
    if dtype == 'sqlite':
        return Sqlite(*args, **kwargs)
    elif dtype == 'mysql':
        return MySQL(*args, **kwargs)
    return None

def sqlite2mysql(sqlitefile, *args, **kwargs):
    sqlite = Sqlite(sqlitefile)
    sqlite.open()

    mysql = MySQL(*args, **kwargs)
    mysql.open()
    for table in sqlite.show_tables():
        print table
        schema = sqlite._schema(table)
        mysql.bind_table(table, schema)
        records = sqlite.select('*', __table__=table)
        for record in records:
            mysql.insert_record(__table__=table, **record)
        mysql.commit()
    mysql.close()

    sqlite.close()

def mysql2sqlite(sqlitefile, *args, **kwargs):
    priv_tables = [
      'columns_priv', 'db', 'event', 'func', 'general_log', 'help_category',
      'help_keyword', 'help_relation', 'help_topic', 'innodb_index_stats',
      'innodb_table_stats', 'ndb_binlog_index', 'plugin', 'proc', 'procs_priv',
      'proxies_priv', 'servers', 'slave_master_info', 'slave_relay_log_info',
      'slave_worker_info', 'slow_log', 'tables_priv', 'time_zone', 'time_zone_leap_second',
      'time_zone_name', 'time_zone_transition', 'time_zone_transition_type', 'user'
    ]

    mysql = MySQL(*args, **kwargs)
    mysql.open()

    sqlite = Sqlite(sqlitefile)
    sqlite.open()
    for table in mysql.show_tables():
        if table in priv_tables:
            continue
        schema = mysql._schema(table)
        print table, schema
        sqlite.bind_table(table, schema)
        records = mysql.select('*', __table__=table)
        for record in records:
            sqlite.insert_record(__table__=table, **record)
        sqlite.commit()
    sqlite.close()

    mysql.close()

def test():
    sqlite = Sqlite('mydb.d3', True)
    sqlite.open()
    for table in sqlite.show_tables():
        sqlite.drop_table(table)
    sqlite.bind_table('clients', 'id INT PRIMARY KEY, name CHAR(60)')
    sqlite.delete_record()
    sqlite.insert_record(5, 'John')
    sqlite.insert_record(id=9, name='LuLu')
    print sqlite.select(['id'], 'order by id desc')
    print sqlite.desc('clients')

    info = [
        (1, 'LiLi'),
        (2, 'Tom'),
        (3, 'Xiao'),
        (4, 'Joo')
    ]
    sqlite.insert_records(info)
    print sqlite.select('*')

    sqlite.delete_record('where id = 9')
    print sqlite.select('*')
    sqlite.close()

    mysql = MySQL('localhost', 'root', '', 'mysql')
    mysql.open()
    mysql.drop_table('clients')
    mysql.close()
    sqlite2mysql('mydb.d3', 'localhost', 'root', '', 'mysql')

    sqlite = Sqlite('mydb.d3')
    sqlite.open()
    for table in sqlite.show_tables():
        sqlite.drop_table(table)
    sqlite.close()
    mysql2sqlite('mydb.d3', 'localhost', 'root', '', 'mysql')

    sqlite = Sqlite('mydb.d3')
    sqlite.open()
    for table in sqlite.show_tables():
        print table, ':', sqlite.select('*', __table__=table)
    sqlite.close()

def test2():
    sqlite = Sqlite('mydb.d3', True)
    sqlite.open()
    sqlite.bind_table('datep', 'id INT PRIMARY KEY, date timestamp NOT NULL DEFAULT current_timestamp')
    sqlite.delete_record()
    sqlite.insert_record(id=5)
    print sqlite.select(['*'], 'order by id desc')
    print sqlite.desc('datep')
    sqlite.close()

if __name__ == '__main__':
    import os, sys
    parent = os.path.split(os.path.dirname(os.path.abspath(sys.argv[0])))[0]
    if parent not in sys.path:
        sys.path.insert(0, parent)
    import kit
    try:
        test()
        test2()
    except:
        kit.print_exc_plus()
