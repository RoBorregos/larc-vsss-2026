GPU_MODEL=$(nvidia-smi --query-gpu=name --format=csv,noheader | grep -o -E '4050|5050')

echo "Building with model $GPU_MODEL"

docker build --build-arg GPU_MODEL="$GPU_MODEL" -t roborregos/vsss:vision .