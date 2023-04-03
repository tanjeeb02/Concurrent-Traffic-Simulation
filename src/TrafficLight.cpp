#include <iostream>
#include <random>
#include "memory.h"
#include "TrafficLight.h"

/* Implementation of class "MessageQueue" */

template <typename T>
T MessageQueue<T>::receive()
{
    std::unique_lock<std::mutex> uLock(_mutex);
    _cond.wait(uLock, [this] { return !_queue.empty(); }); // pass unique lock to condition variable

    T msg = std::move(_queue.back());
    _queue.clear();

    return msg;
}

template <typename T>
void MessageQueue<T>::send(T &&msg)
{
    std::lock_guard<std::mutex> uLock(_mutex);
    _queue.push_back(std::move(msg));
    _cond.notify_one();
}

/* Implementation of class "TrafficLight" */

TrafficLight::TrafficLight()
{
    _currentPhase = TrafficLightPhase::red;
}

void TrafficLight::waitForGreen()
{
    while (true)
    {
        if (_messageQueue.receive() == TrafficLightPhase::green)
        {
            return;
        }
    }
}

TrafficLightPhase TrafficLight::getCurrentPhase()
{
    return _currentPhase;
}

void TrafficLight::simulate()
{
    threads.emplace_back(std::thread(&TrafficLight::cycleThroughPhases, this));
}

// virtual function which is executed in a thread
void TrafficLight::cycleThroughPhases()
{
    // initialize time variables
    std::chrono::time_point<std::chrono::system_clock> lastUpdate = std::chrono::system_clock::now();
    std::chrono::duration<double> elapsedSeconds;

    // generate random cycle duration between 4 and 6 seconds
    std::random_device rd;
    std::mt19937 eng(rd());
    std::uniform_real_distribution<double> distr(4.0, 6.0);
    double cycleDuration = distr(eng);

    // start cycle
    while (true)
    {

        // calculate time difference to stop watch
        std::chrono::time_point<std::chrono::system_clock> currentUpdate = std::chrono::system_clock::now();
        elapsedSeconds = std::chrono::duration_cast<std::chrono::duration<double>>(currentUpdate - lastUpdate);

        // check if time difference is greater than 4 seconds
        if (elapsedSeconds.count() >= cycleDuration)
        {
            // toggle traffic light phase
            if (_currentPhase == TrafficLightPhase::red)
            {
                _currentPhase = TrafficLightPhase::green;
            }
            else
            {
                _currentPhase = TrafficLightPhase::red;
            }

            // send update message to message queue using move semantics
            _messageQueue.send(std::move(_currentPhase));

            // reset stop watch for next cycle
            lastUpdate = std::chrono::system_clock::now();

            // generate new random cycle duration between 4 and 6 seconds
            cycleDuration = distr(eng);
        }

        // sleep for 1 ms
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}
