from flask import Flask, jsonify, request
from collections import deque
import os, logging

# 전역 변수
map_size = None # 그래드 맵 사이즈
global grid # 그리드 맵

# 초기화
def configure_logging():
    logger = logging.getLogger("ServerLogger")
    if not logger.handlers:
        logger.setLevel(logging.INFO)
        fh = logging.FileHandler("MovingObjectDateTime.log", encoding='utf-8')
        ch = logging.StreamHandler()
        formatter = logging.Formatter('[%(asctime)s, %(levelname)s] %(message)s', "%Y-%m-%d %H:%M:%S")
        fh.setFormatter(formatter)
        ch.setFormatter(formatter)
        logger.addHandler(fh)
        logger.addHandler(ch)
    return logger

def create_app():
    app = Flask(__name__)
    app.config['map_size'] = None
    app.config['grid'] = []
    return app

logger = configure_logging()
app = create_app()



# 함수 목록
def get_map_size_input():
    while True:
        try:
            input_value = int(input("Please enter the initial value for N (minimum: 5, maximum: 10): "))
            if 5 <= input_value <= 10:
                return input_value
            else:
                print("N must be between 5 and 10. Please enter again.")
        except ValueError:
            print("Invalid input. Please enter a number.")

def initialize_map_size():
    global map_size
    # 개발 중에 Flask가 자동으로 리로드될 때 중복 실행을 방지
    if os.environ.get("WERKZEUG_RUN_MAIN") == "true" and map_size is None:
        map_size = get_map_size_input()
        create_grid(map_size)
        print(f"Map size set to {map_size}")

def create_grid(size):
    global grid
    grid = [[0 for _ in range(size)] for _ in range(size)]

def bfs_path_finding(grid_dimension, start_point, end_point):
    def within_grid_limits(coordinates):
        return 0 <= coordinates[0] < grid_dimension and 0 <= coordinates[1] < grid_dimension
    def can_move_to(coordinates):
        return grid[coordinates[0]][coordinates[1]] == 0

    # 이동 방향 : 상하좌우
    MOVE_SET = [(-1, 0), (1, 0), (0, -1), (0, 1)]

    # 큐 초기화 후 시작점 설정
    queue = deque([(start_point, [start_point])])  # (현재 위치, 경로)
    visited = set()
    visited.add(start_point) 

    while queue:
        current_position, path = queue.popleft()
        if current_position == end_point:
            # 서버는 (0,0) ~ (N-1, N-1) 범위로 계산하지만
            # 클라이언트는 (1,1) ~ (N,N) 를 사용하기 때문에 +1
            return [(x+1, y+1) for x, y in path]

        for move in MOVE_SET:
            next_step = (current_position[0] + move[0], current_position[1] + move[1])
            if within_grid_limits(next_step) and can_move_to(next_step) and next_step not in visited:
                visited.add(next_step) 
                queue.append((next_step, path + [next_step])) 

    return []  # 경로가 없을 시

def validate_json_keys(data, required_keys):
    if not data:
        return {"status": "error", "message": "Invalid or missing JSON payload"}, 400

    missing_keys = [key for key in required_keys if key not in data]
    if missing_keys:
        return {"status": "error", "message": f"Missing keys: {', '.join(missing_keys)}"}, 400

    return None

# Route: 그리드 크기 확인용
@app.route('/')
def home():
    return f"Grid Map Size: {map_size}"

# Route: grid 크기 Get 요청
@app.route('/size', methods=['GET'])
def get_n():
    return jsonify({"size": map_size})

# Route: 경로 Post 요청
@app.route('/path', methods=['POST'])
def get_path():
    try:
        data = request.get_json()

        # keys 유효성 체크
        validation_error = validate_json_keys(data, ['id', 'InitialPosX', 'InitialPosY', 'TargetPosX', 'TargetPosY'])
        if validation_error:
            return jsonify(validation_error[0]), validation_error[1]

        # 데이터 추출
        moving_id = data['id']
        initial_pos_x = data['InitialPosX']
        initial_pos_y = data['InitialPosY']
        target_pos_x = data['TargetPosX']
        target_pos_y = data['TargetPosY']

        # 범위 유효성 체크 (1 <= x, y <= map_size)
        if not (1 <= initial_pos_x <= map_size and 1 <= initial_pos_y <= map_size):
            return jsonify({
                "status": "error",
                "message": "Initial position is out of bounds."
            }), 400
        if not (1 <= target_pos_x <= map_size and 1 <= target_pos_y <= map_size):
            return jsonify({
                "status": "error",
                "message": "Target position is out of bounds."
            }), 400

        print(f"id:{moving_id}, initial:({initial_pos_x},{initial_pos_y}), target:({target_pos_x},{target_pos_y})")
        # (0,0)~(N-1,N-1) 형식으로 전환
        start = (initial_pos_x-1, initial_pos_y-1)
        goal = (target_pos_x-1, target_pos_y-1)
        path = bfs_path_finding(map_size, start, goal)

        if not path:
            return jsonify({
                "status": "error",
                "message": "Path not found."
            }), 404
        return jsonify({
            "status": "success",
            "id": moving_id,
            "path": path
        }), 200

    
    except KeyError as e:
        return jsonify({
            "status": "error",
            "message": f"Missing key in input data: {str(e)}"
        }), 400
    except Exception as e:
        return jsonify({
            "status": "error",
            "message": f"An unexpected error occurred: {str(e)}"
        }), 500


# Route: 로그 저장 Post 요청
@app.route('/log', methods=['POST'])
def log_message():
    try:
        # Data extraction
        data = request.json
        moving_object_id = data.get('moving_object_id')
        start_time = data.get('start_time')
        end_time = data.get('end_time')

        # keys 체크
        validation_error = validate_json_keys(data, ['moving_object_id', 'start_time', 'end_time'])
        if validation_error:
            return jsonify(validation_error[0]), validation_error[1]

        # type 체크
        if not isinstance(moving_object_id, (str, int)):
            return jsonify({
                "status": "error",
                "message": "Invalid data type for moving_object_id"
            }), 400

        # message format 지정
        message = f"MovingObjectID: {moving_object_id}, START_TIME:{start_time}, END_TIME:{end_time}"

        logger.info(message)
        return jsonify({
            "status": "success",
            "message": "Message logged successfully"
        }), 200

    except Exception as e:
        logger.error(f"An error occurred while processing the log: {str(e)}")
        return jsonify({
            "status": "error",
            "message": f"An unexpected error occurred: {str(e)}"
        }), 500



if __name__ == "__main__":
    initialize_map_size()
    app.run(debug=True)