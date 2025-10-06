#!/bin/bash

echo "=== BoundedQueue Benchmark Results ==="
echo ""
echo "Balanced Producers/Consumers:"
printf "%-12s %-12s %-12s %-15s %-20s\n" "Producers" "Consumers" "Capacity" "Duration(ms)" "Throughput(items/s)"
echo "--------------------------------------------------------------------------------"

for threads in 1 2 4 8; do
    output=$(./driver $threads $threads 100000 50 2>/dev/null)
    duration=$(echo "$output" | grep "Duration:" | awk '{print $2}')
    throughput=$(echo "$output" | grep "Throughput:" | awk '{print $2}')
    printf "%-12s %-12s %-12s %-15s %-20s\n" "$threads" "$threads" "50" "$duration" "$throughput"
done

echo ""
echo "Varying Queue Capacity (5 producers, 5 consumers):"
printf "%-12s %-20s\n" "Capacity" "Throughput(items/s)"
echo "----------------------------------------"

for cap in 5 10 50 100 500; do
    output=$(./driver 5 5 100000 $cap 2>/dev/null)
    throughput=$(echo "$output" | grep "Throughput:" | awk '{print $2}')
    printf "%-12s %-20s\n" "$cap" "$throughput"
done