#include <iostream>
#include <random>



class ErrorProb {
protected:
  double threshold_;
#ifndef GEN_TRUE_RANDOM
  std::default_random_engine generator_;
#else
  std::random_device generator_;
#endif
public:
  virtual bool GenErrorVal(void)=0;

  ErrorProb(void) 
    : threshold_(0.0) {}
  ErrorProb(double threshold) 
    : threshold_(threshold) {}
  virtual ~ErrorProb(void) {}
};

class UniformRandom : public ErrorProb {
protected:
  std::uniform_real_distribution<double> distribution_;
  double low_;
  double high_;
public:
  bool GenErrorVal(void) {
    distribution_.reset();
    double error = distribution_(generator_);
    return error >= threshold_;
  }

  void TestErrorProb(int num_bucket=10) {
    std::cout << "Uniform Randomi, low_ : " << low_ <<", high : "<< high_ << std::endl;
    const int nrolls = 10000;  // number for test
    const int nstars = 100;    // maximum number of stars to distribute
    int bucket[num_bucket];
    
    for (int i=0; i<num_bucket; ++i) {
      bucket[i] = 0;
    }

    for (int i=0; i<nrolls; ++i) {
      double number = distribution_(generator_);
      if(number < 0) std::cout << "negative : " << number << std::endl;
      if ((number>=0.0) && (number<static_cast<double>(num_bucket))) 
        ++bucket[static_cast<int>(number)];
    } 

    for (int i=0; i<num_bucket; ++i) {
      std::cout << i << "-" << (i+1) << ":\t";
      std::cout << std::string(bucket[i]*nstars/nrolls, '*') << std::endl;
    }

  }

  UniformRandom(void) 
    : distribution_(0.0, 1.0), low_(0.0), high_(1.0) {}
  UniformRandom(double low, double high) 
    : distribution_(low, high), low_(low), high_(high) {}
  virtual ~UniformRandom() {}
};

class LogNormal : public ErrorProb {
protected:
  std::lognormal_distribution<double> distribution_;
  double mean_;
  double std_;
public:
  bool GenErrorVal(void) {
    distribution_.reset();
    double error = distribution_(generator_);
    return error >= threshold_;
  }

  void TestErrorProb(int num_bucket=10) {
    std::cout << "Log Normal, mean : " << mean_ << ", std : " << std_ << std::endl;
    const int nrolls = 10000;  // number for test
    const int nstars = 100;    // maximum number of stars to distribute
    int bucket[num_bucket];

    for (int i=0; i<num_bucket; ++i) {
      bucket[i] = 0;
    }
    for (int i=0; i<nrolls; ++i) {
      double number = distribution_(generator_);
      if(number < 0) std::cout << "negative : " << number << std::endl;
      if ((number>=0.0) && (number<static_cast<double>(num_bucket))) 
        ++bucket[static_cast<int>(number)];
    } 

    for (int i=0; i<num_bucket; ++i) {
      std::cout << i << "-" << (i+1) << ":\t";
      std::cout << std::string(bucket[i]*nstars/nrolls, '*') << std::endl;
    }

  }
  LogNormal(void) 
    : distribution_(0.0, 1.0), mean_(0.0), std_(1.0) {}
  LogNormal(double mean, double std) 
    : distribution_(mean, std), mean_(mean), std_(std) {}
  virtual ~LogNormal() {}
};

class Exponential : public ErrorProb {
protected:
  std::exponential_distribution<double> distribution_;
  double lamda_;
public:
  bool GenErrorVal(void) {
    distribution_.reset();
    double error = distribution_(generator_);
    return error >= threshold_;
  }

  void TestErrorProb(int num_bucket=10) {
    std::cout << "Exponential, lamda : " << lamda_ << std::endl;
    const int nrolls = 10000;  // number for test
    const int nstars = 100;    // maximum number of stars to distribute
    int bucket[num_bucket];

    for (int i=0; i<num_bucket; ++i) {
      bucket[i] = 0;
    }
    for (int i=0; i<nrolls; ++i) {
      double number = distribution_(generator_);
      if(number < 0) std::cout << "negative : " << number << std::endl;
      if ((number>=0.0) && (number<static_cast<double>(num_bucket))) 
        ++bucket[static_cast<int>(number)];
    } 

    for (int i=0; i<num_bucket; ++i) {
      std::cout << i << "-" << (i+1) << ":\t";
      std::cout << std::string(bucket[i]*nstars/nrolls, '*') << std::endl;
    }

  }
  Exponential(void) 
    : distribution_(1.0), lamda_(1.0) {}

  Exponential(double lamda) 
    : distribution_(lamda), lamda_(lamda) {}

  virtual ~Exponential() {}
};

class Normal : public ErrorProb {
protected:
  std::normal_distribution<double> distribution_;
  double mean_;
  double std_;
public:
  bool GenErrorVal(void) {
    distribution_.reset();
    double error = distribution_(generator_);
    if(error < 0) error *= -1;
    return error >= threshold_;
  }

  void TestErrorProb(int num_bucket=10) {
    std::cout << "Normal mean : " << mean_ << ", std : " << std_ << std::endl;
    const int nrolls = 10000;  // number for test
    const int nstars = 100;    // maximum number of stars to distribute
    int bucket[num_bucket];

    for (int i=0; i<num_bucket; ++i) {
      bucket[i] = 0;
    }
    for (int i=0; i<nrolls; ++i) {
      double number = distribution_(generator_);
      if(number < 0) number *= -1;
      if(number < 0) std::cout << "negative : " << number << std::endl;

      if ((number>=0.0) && (number<static_cast<double>(num_bucket))) 
        ++bucket[static_cast<int>(number)];
    } 

    for (int i=0; i<num_bucket; ++i) {
      std::cout << i << "-" << (i+1) << ":\t";
      std::cout << std::string(bucket[i]*nstars/nrolls, '*') << std::endl;
    }

  }
  Normal(void) 
    : distribution_(0.0, 1.0), mean_(0.0), std_(1.0) {}
  Normal(double mean, double std) 
    : distribution_(mean, std), mean_(mean), std_(std) {}
  virtual ~Normal() {}
};

class Poisson : public ErrorProb {
protected:
  std::poisson_distribution<int> distribution_;
  double mean_;
public:
  bool GenErrorVal(void) {
    int error = distribution_(generator_);
    return static_cast<double>(error) >= threshold_;
  }
  
  void TestErrorProb(int num_bucket=10) {
    std::cout << "Poisson mean : " << mean_ << std::endl;
    const int nrolls = 10000;  // number for test
    const int nstars = 100;    // maximum number of stars to distribute
    int bucket[num_bucket];

    for (int i=0; i<num_bucket; ++i) {
      bucket[i] = 0;
    }
    for (int i=0; i<nrolls; ++i) {
      double number = distribution_(generator_);
      if(number < 0) std::cout << "negative : " << number << std::endl;
      if ((number>=0.0) && (number<static_cast<double>(num_bucket))) 
        ++bucket[static_cast<int>(number)];
    } 

    for (int i=0; i<num_bucket; ++i) {
      std::cout << i << "-" << (i+1) << ":\t";
      std::cout << std::string(bucket[i]*nstars/nrolls, '*') << std::endl;
    }

  }

  Poisson(void) 
    : distribution_(1.0), mean_(1.0) {}
  Poisson(double mean) 
    : distribution_(mean), mean_(mean) {}
  virtual ~Poisson() {}
};

