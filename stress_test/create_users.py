import mysql.connector

# --- 数据库配置 ---
db_config = {
    "host": "127.0.0.1",
    "user": "root",
    "password": "123456", 
    "database": "chat",
    "ssl_disabled": True  
}

def create_test_users(count):
    conn = None
    try:
        conn = mysql.connector.connect(**db_config)
        cursor = conn.cursor()

        print(f"正在准备插入 {count} 条用户数据...")
        
        # 批量插入
        sql = "INSERT INTO User(name, password, state) VALUES (%s, %s, %s)"
        users = []
        for i in range(1, count + 1):
            users.append((f"test_user{i}", "123456", "offline"))

        cursor.executemany(sql, users)
        conn.commit()
        print(f"成功插入 {cursor.rowcount} 条数据！")

    except mysql.connector.Error as err:
        print(f"数据库错误: {err}")
    except Exception as e:
        print(f"其他错误: {e}")
    finally:
        # 修改这里的判断，防止报错
        if conn and conn.is_connected():
            cursor.close()
            conn.close()

if __name__ == "__main__":
    create_test_users(100000)