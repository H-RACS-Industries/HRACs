import sqlite3
class DatabaseManager:
    def __init__(self, name: str):
        self.connection = sqlite3.connect(name)
        self.cursor = self.connection.cursor()
    def execute(self, query, values):
        result = self.cursor.execute(query, values).fetchall()
        return result

    def close(self):
        self.connection.close()

    def run_save(self, query, values):
        self.cursor.execute(query, values)
        self.connection.commit()
