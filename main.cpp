#include <iostream>
#include <cmath>
#include <vector>
#include <fstream>
#include <random>

#define INNODE      2
#define HIDENODE    4
#define OUTNODE     1

double learning_rate = 0.8;  //学习率
double threshold = 1e-4;    //最大误差
double mosttime = 1e6;      //最大迭代次数

struct Sample {
    std::vector<double> in, out;
}; //数据样本

struct Node {
    double value{}, bias{}, bias_delta{};
    std::vector<double> weight, weight_delta;
};  //神经元模型

namespace utils {
    inline double sigmoid(double x) {
        double res = 1.0 / (1 + std::exp(-x));
        return res;
    }

    std::vector<double> getFileData(std::string filename) {
        std::vector<double> res;

        std::ifstream in(filename);
        if (in.is_open()) {
            while (!in.eof()) {
                double buffer;
                in >> buffer;
                res.push_back(buffer);
            }
            in.close();
        } else {
            std::cout << "Error in reading " << filename << std::endl;
        }
        return res;
    }

    std::vector<Sample> getTrainData(std::string filename) {
        /*
         * 获取训练集数据
         * */
        std::vector<Sample> res;
        std::vector<double> buffer = getFileData(filename);
        for (size_t i = 0; i < buffer.size(); i += INNODE + OUTNODE) {
            Sample tmp;
            for (size_t t = 0; t < INNODE; t++) {
                tmp.in.push_back(buffer[i + t]);      //将前2个数据放入tmp.in中，这里是数据集输入
            }
            for (size_t t = 0; t < OUTNODE; t++) {
                tmp.out.push_back(buffer[i + INNODE + t]); //数据集输出
            }
            res.push_back(tmp);
        }
        return res;
    }

    std::vector<Sample> getTestData(std::string filename) {
        /*
         * 获取测试集数据
         * */
        std::vector<Sample> res;
        std::vector<double> buffer = getFileData(filename);
        for (size_t i = 0; i < buffer.size(); i += INNODE) {
            Sample tmp;
            for (size_t t = 0; t < INNODE; t++) {
                tmp.in.push_back(buffer[i + t]);      //将前2个数据放入tmp.in中，这里是数据集输入
            }
            res.push_back(tmp);
        }
    }
} //工具函数命名空间

//定义网络层
Node *inputLayer[INNODE], *hideLayer[HIDENODE], *outLayer[OUTNODE];

inline void initNN() {
    /*
     * 神经网络初始化
     * */
    std::mt19937 rd;
    rd.seed(std::random_device()());
    std::uniform_real_distribution<double> distribution(-1, 1);
    for (size_t i = 0; i < INNODE; i++) {
        ::inputLayer[i] = new Node();
        for (size_t j = 0; j < HIDENODE; j++) {
            ::inputLayer[i]->weight.push_back(distribution(rd));
            ::inputLayer[i]->weight_delta.push_back(0.f);
        }
    }

    for (size_t i = 0; i < HIDENODE; i++) {
        ::hideLayer[i] = new Node();
        ::hideLayer[i]->bias = distribution(rd);
        for (size_t j = 0; j < OUTNODE; j++) {
            ::hideLayer[i]->weight.push_back(distribution(rd));
            ::hideLayer[i]->weight_delta.push_back(0.f);
        }
    }

    for (size_t i = 0; i < OUTNODE; i++) {
        ::outLayer[i] = new Node();
        ::outLayer[i]->bias = distribution(rd);
    }
}

inline void reset_delta() {
    for (size_t i = 0; i < INNODE; i++) {
        ::inputLayer[i]->weight_delta.assign(::inputLayer[i]->weight_delta.size(), 0.f);
    }
    for (size_t i = 0; i < HIDENODE; i++) {
        ::hideLayer[i]->bias_delta = 0.f;
        ::hideLayer[i]->weight_delta.assign(::hideLayer[i]->weight_delta.size(), 0.f);
    }
    for (size_t i = 0; i < OUTNODE; i++) {
        ::outLayer[i]->bias_delta = 0.f;
    }
}

int main(int argc, char *argv[]) {

    initNN();
    std::vector<Sample> train_data = utils::getTrainData("E:\\C++_File\\BPNN\\traindata.txt");

    for (size_t times = 0; times < mosttime; times++) {
        reset_delta();

        double error_max = 0.f;

        for (auto &idx : train_data) {
            /*
             * 训练
             * */
            for (size_t i = 0; i < INNODE; i++) {
                ::inputLayer[i]->value = idx.in[i];
            }
            //正向传播
            for (size_t j = 0; j < HIDENODE; j++) {
                double sum = 0;
                for (size_t i = 0; i < INNODE; i++) {
                    sum += ::inputLayer[i]->value * ::inputLayer[i]->weight[j];
                }
                sum -= hideLayer[j]->bias;
                ::hideLayer[j]->value = utils::sigmoid(sum);
            }

            for (size_t j = 0; j < OUTNODE; j++) {
                double sum = 0;
                for (size_t i = 0; i < HIDENODE; i++) {
                    sum += ::hideLayer[i]->value * ::hideLayer[i]->weight[j];
                }
                sum -= ::outLayer[j]->bias;
                ::outLayer[j]->value = utils::sigmoid(sum);
            }

            // 计算Loss
            double error = 0.f;
            for (size_t i = 0; i < OUTNODE; i++) {
                double tmp = std::fabs(::outLayer[i]->value - idx.out[i]);
                error += (tmp * tmp) / 2;
            }

            error_max = std::max(error_max, error);

            //反向传播
            for (size_t i = 0; i < OUTNODE; i++) {
                double bias_delta = -(idx.out[i] - ::outLayer[i]->value) *
                                    ::outLayer[i]->value * (1.0 - ::outLayer[i]->value);
                ::outLayer[i]->bias_delta += bias_delta;
            }

            for (size_t i = 0; i < HIDENODE; i++) {
                for (size_t j = 0; j < OUTNODE; j++) {
                    double weight_delta = (idx.out[j] - ::outLayer[j]->value) *
                                          ::outLayer[j]->value * (1.0 - ::outLayer[j]->value) *
                                          ::hideLayer[i]->value;
                    ::hideLayer[i]->weight_delta[j] += weight_delta;
                }
            }

            for (size_t i = 0; i < HIDENODE; i++) {
                double sum = 0;
                for (size_t j = 0; j < OUTNODE; j++) {
                    sum += -(idx.out[j] - ::outLayer[j]->value) *
                           ::outLayer[j]->value * (1.0 - ::outLayer[j]->value) *
                           ::hideLayer[i]->weight[j] *
                           ::hideLayer[i]->value * (1.0 - ::hideLayer[i]->value);
                }
                ::hideLayer[i]->bias_delta += sum;
            }

            for (size_t i = 0; i < INNODE; i++) {
                for (size_t j = 0; j < HIDENODE; j++) {
                    double sum = 0.f;
                    for (size_t k = 0; k < OUTNODE; k++) {
                        sum += (idx.out[k] - ::outLayer[k]->value) *
                               ::outLayer[k]->value * (1.0 - outLayer[k]->value) *
                               ::hideLayer[j]->weight[k] *
                               ::hideLayer[j]->value * (1.0 - hideLayer[j]->value) *
                               inputLayer[i]->value;
                    }
                    ::inputLayer[i]->weight_delta[j] += sum;
                }
            }
        }

        if (error_max < ::threshold) {
            std::cout << "Success with " << times + 1 << "times training" << std::endl;
            std::cout << "Maximum Error: " << error_max << std::endl;
            break;
        }

//        std::cout << "Loss: " << error_max << std::endl;

        auto train_data_size = double(train_data.size());

        for (size_t i = 0; i < INNODE; i++) {
            for (size_t j = 0; j < HIDENODE; j++) {
                ::inputLayer[i]->weight[j] +=
                        learning_rate * ::inputLayer[i]->weight_delta[j] / train_data_size;
            }
        }

        for (size_t i = 0; i < HIDENODE; i++) {
            ::hideLayer[i]->bias +=
                    learning_rate * ::hideLayer[i]->bias_delta / train_data_size;
            for (size_t j = 0; j < OUTNODE; j++) {
                ::hideLayer[i]->weight[j] +=
                        learning_rate * ::hideLayer[i]->weight_delta[j] / train_data_size;
            }
        }

        for (size_t i = 0; i < OUTNODE; i++) {
            ::outLayer[i]->bias +=
                    learning_rate * ::outLayer[i]->bias_delta / train_data_size;
        }
    }

    std::vector<Sample> test_data = utils::getTestData("E://C++_File//BPNN//testdata.txt");

// predict
    for (auto &idx : test_data) {

        for (size_t i = 0; i < INNODE; i++) {
            ::inputLayer[i]->value = idx.in[i];
        }

        for (size_t j = 0; j < HIDENODE; j++) {
            double sum = 0;
            for (size_t i = 0; i < INNODE; i++) {
                sum += ::inputLayer[i]->value * inputLayer[i]->weight[j];
            }
            sum -= ::hideLayer[j]->bias;

            ::hideLayer[j]->value = utils::sigmoid(sum);
        }

        for (size_t j = 0; j < OUTNODE; j++) {
            double sum = 0;
            for (size_t i = 0; i < HIDENODE; i++) {
                sum += ::hideLayer[i]->value * ::hideLayer[i]->weight[j];
            }
            sum -= ::outLayer[j]->bias;

            ::outLayer[j]->value = utils::sigmoid(sum);

            idx.out.push_back(::outLayer[j]->value);

            for (auto &tmp : idx.in) {
                std::cout << tmp << " ";
            }
            for (auto &tmp : idx.out) {
                std::cout << tmp << " ";
            }
            std::cout << std::endl;
        }
        return 0;
    }
}