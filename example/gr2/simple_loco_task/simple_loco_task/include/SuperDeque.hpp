/**
 * @file SuperDeque.hpp
 * @brief Header file for SuperDeque in robot controllers.
 *
 * @details 
 *
 * @author renminwansui
 * @date April 30, 2025
 * @version v1.0
 * @bug No known bugs
 *
 * @copyright Copyright (c) 2025, shanghai fourier intelligence
 */

 #pragma once


#include <deque>
#include <any>
#include <vector>
#include <Eigen/Dense>
#include <iostream>
//右进左出，固定长度的队列,有需要请自行添加目前仅支持左值的double,Eigen::Vector2d\3d\4d\Xd
class SuperDeque {
public:
    SuperDeque(int len, const std::any& obj) {
        _len = len;
        _deque.resize(_len,obj);
    }
    void push(const std::any& input) {
        _deque.push_back(input);
        while(_deque.size() > _len) {
            _deque.pop_front();
        }
    }

    std::vector<double> toDoubleVector() {
        std::vector<double> result;
        for (const auto& item : _deque) {
            if (item.type() == typeid(Eigen::Vector2d)) {
                auto vec = std::any_cast<Eigen::Vector2d>(item);
                auto temp = std::vector<double>(vec.data(), vec.data() + vec.size());
                result.insert(result.end(), temp.begin(), temp.end());
            } else if (item.type() == typeid(Eigen::Vector3d)) {
                auto vec = std::any_cast<Eigen::Vector3d>(item);
                auto temp = std::vector<double>(vec.data(), vec.data() + vec.size());
                result.insert(result.end(), temp.begin(), temp.end());
            } else if (item.type() == typeid(Eigen::Vector4d)) {
                auto vec = std::any_cast<Eigen::Vector4d>(item);
                auto temp = std::vector<double>(vec.data(), vec.data() + vec.size());
                result.insert(result.end(), temp.begin(), temp.end());
            } else if (item.type() == typeid(Eigen::VectorXd)) {
                auto vec = std::any_cast<Eigen::VectorXd>(item);
                auto temp = std::vector<double>(vec.data(), vec.data() + vec.size());
                result.insert(result.end(), temp.begin(), temp.end());
            } else if (item.type() == typeid(double)) {
                result.push_back(std::any_cast<double>(item));
            } else {
                std::cerr << "item type error! " <<item.type().name()<< std::endl;
                result.clear();
                break;
            }
        }
        return result;
    }
private:
    std::deque<std::any> _deque;
    int _len;
};

