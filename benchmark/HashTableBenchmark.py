import time

def hash(x, z):
    return (x << 32) | z

table = {}
start_time = time.time_ns()
for i in range(1000000):
    key = hash(i, i)
    table[key] = i
for i in range(1000000):
    key = hash(i, i)
    value = table[key]
    del table[key]
    new_key = hash(i, i + 1)
    table[new_key] = value
end_time = time.time_ns()
duration = end_time - start_time
print(f"Time taken to insert, retrieve, delete, and add 1 million entries: {duration} nanoseconds")
