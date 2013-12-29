#ifndef _FEATURE_TRANSFORM_H_
#define _FEATURE_TRANSFORM_H_

#include <device_matrix.h>
#include <device_math.h>

#define matlog(x) { cout << #x << " = [" << endl; x.print(); cout << "];" << endl; }

typedef device_matrix<float> mat;


template <typename T>
device_matrix<T> operator & (const device_matrix<T>& A, const device_matrix<T>& B) {
  assert(A.getRows() == B.getRows() && A.getCols() == B.getCols());

  device_matrix<T> C(A.getRows(), A.getCols());

  thrust::device_ptr<T> aPtr(A.getData());
  thrust::device_ptr<T> bPtr(B.getData());
  thrust::device_ptr<T> cPtr(C.getData());

  thrust::transform(aPtr, aPtr + A.size(), bPtr, cPtr, thrust::multiplies<T>());

  return C;
}

/*class FeatureTransform {
public:
  virtual void feedForward(mat& fout, const mat& fin, size_t offset, size_t nData) { }
  virtual void backPropagate(mat& delta, mat& fin) { }
};*/

class AffineTransform /*: protected FeatureTransform*/ {
public:
  AffineTransform();
  AffineTransform(const AffineTransform& source);
  AffineTransform(const mat& w);
  AffineTransform(size_t rows, size_t cols);

  AffineTransform& operator = (AffineTransform rhs);
  void setOutputLayer(bool flag);

  mat& getW();
  const mat& getW() const;
  mat& getDw();
  const mat& getDw() const;

  void update(float learning_rate);
  void resize(size_t rows, size_t cols);

  virtual string toString() const;
  virtual void feedForward(mat& fout, const mat& fin, size_t offset, size_t nData);
  virtual void backPropagate(const mat& fin, const mat& fout, mat& error);

  friend void swap(AffineTransform& lhs, AffineTransform& rhs);

protected:
  bool _isOutputLayer;
  mat _w;
  mat _dw;
};

class Softmax : public AffineTransform {
public:
  Softmax(const mat& w);
  Softmax(size_t rows, size_t cols);

  Softmax& operator = (Softmax rhs);
  
  virtual string toString() const;
  virtual void feedForward(mat& fout, const mat& fin, size_t offset, size_t nData);
  virtual void backPropagate(const mat& fin, const mat& fout, mat& error);

  friend void swap(Softmax& lhs, Softmax& rhs);
};

namespace ext {
  template <typename T>
  device_matrix<T> b_sigmoid(const device_matrix<T>& x) {
    device_matrix<T> s(x.getRows(), x.getCols() + 1);
    
    thrust::device_ptr<T> xPtr(x.getData());
    thrust::device_ptr<T> sPtr(s.getData());

    // Leave last column in s untouched
    thrust::transform(xPtr, xPtr + x.size(), sPtr, func::sigmoid<float>());

    // Fill last column in s with 1.0
    thrust::fill(sPtr + s.size() - s.getRows(), sPtr + s.size(), (float) 1.0);

    return s;
  }

  template <typename T>
  device_matrix<T> sigmoid(const device_matrix<T>& x) {
    device_matrix<T> s(x.getRows(), x.getCols());

    thrust::device_ptr<T> xPtr(x.getData());
    thrust::device_ptr<T> sPtr(s.getData());

    thrust::transform(xPtr, xPtr + x.size(), sPtr, func::sigmoid<float>());

    return s;
  }

  template <typename T>
  device_matrix<T> softmax(const device_matrix<T>& x) {
    // TODO
    // Do the softmax
    device_matrix<T> s(x.getRows(), x.getCols());

    thrust::device_ptr<T> xPtr(x.getData());
    thrust::device_ptr<T> sPtr(s.getData());

    thrust::transform(xPtr, xPtr + x.size(), sPtr, func::sigmoid<float>());

    return s;
  }
}

template <typename T>
void memcpy2D(device_matrix<T>& dest, const device_matrix<T>& src, size_t r0, size_t c0, size_t h, size_t w, size_t r1, size_t c1) {

  device_matrix<float>::cublas_geam(
      CUBLAS_OP_N, CUBLAS_OP_N,
      h, w,
      1.0, src.getData() + c0 * src.getRows() + r0, src.getRows(),
      0.0, dest.getData(), dest.getRows(),
      dest.getData() + c1 * dest.getRows() + r1, dest.getRows());
}

template <typename T>
void fillLastColumnWith(device_matrix<T>& A, const T value) {
  thrust::device_ptr<T> ptr(A.getData());
  thrust::fill(ptr + A.size() - A.getRows(), ptr + A.size(), value);
}

template <typename T>
device_matrix<T> add_bias(const device_matrix<T>& A) {
  device_matrix<T> B(A.getRows(), A.getCols() + 1);

  B += 1.0;

  device_matrix<T>::cublas_geam(
      CUBLAS_OP_N, CUBLAS_OP_N,
      A.getRows(), A.getCols(),
      1.0, A.getData(), A.getRows(),
      0.0, B.getData(), B.getRows(),
      B.getData(), B.getRows()
  );

  return B;
}


#endif // _FEATURE_TRANSFORM_H_