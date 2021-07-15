#include <AssignmentMunkres.h>
#include <BottleneckDistance.h>
#include <GabowTarjan.h>
#include <Geometry.h>

using ttk::BottleneckDistance;
using ttk::MatchingType;

BottleneckDistance::BottleneckDistance() {
  this->setDebugMsgPrefix("BottleneckDistance");
}

constexpr unsigned long long str2int(const char *str, int h = 0) {
  return !str[h] ? 5381 : (str2int(str, h + 1) * 33) ^ str[h];
}

int BottleneckDistance::execute(const ttk::DiagramType &diag0,
                                const ttk::DiagramType &diag1,
                                std::vector<MatchingType> &matchings,
                                const bool usePersistenceMetric) {
  Timer t;

  bool fromParaView = pvAlgorithm_ >= 0;
  if(fromParaView) {
    switch(pvAlgorithm_) {
      case 0:
        this->printMsg("Solving with the TTK approach");
        this->computeBottleneck(diag0, diag1, matchings, usePersistenceMetric);
        break;
      case 1: {
        this->printMsg("Solving with the legacy Dionysus exact approach.");
        this->printErr("Not supported");
      } break;
      case 2: {
        this->printMsg(
          "Solving with the approximate Dionysus geometric approach.");
        this->printErr("Not supported");
      } break;
      case 3: {
        this->printMsg("Solving with the parallel TTK approach");
        this->printErr("Not supported");
      } break;
      case 4: {
        this->printMsg("Benchmarking");
        this->printErr("Not supported");
      } break;
      default: {
        this->printErr("You must specify a valid assignment algorithm.");
      }
    }

  } else {
    switch(str2int(algorithm_.c_str())) {
      case str2int("0"):
      case str2int("ttk"):
        this->printMsg("Solving with the TTK approach");
        this->computeBottleneck(diag0, diag1, matchings, usePersistenceMetric);
        break;
      case str2int("1"):
      case str2int("legacy"): {
        this->printMsg("Solving with the legacy Dionysus exact approach.");
        this->printErr("Not supported");
      } break;
      case str2int("2"):
      case str2int("geometric"): {
        this->printMsg(
          "Solving with the approximate Dionysus geometric approach.");
        this->printErr("Not supported");
      } break;
      case str2int("3"):
      case str2int("parallel"): {
        this->printMsg("Solving with the parallel TTK approach");
        this->printErr("Not supported");
      } break;
      case str2int("bench"): {
        this->printMsg("Benchmarking");
        this->printErr("Not supported");
      } break;
      default: {
        this->printErr("You must specify a valid assignment algorithm.");
      }
    }
  }

  this->printMsg("Complete", 1, t.getElapsedTime(), threadNumber_);

  return 0;
}

template <typename distFuncType, typename diagFuncType>
void ttk::BottleneckDistance::buildCostMatrices(
  const ttk::DiagramType &CTDiagram1,
  const ttk::DiagramType &CTDiagram2,
  const int d1Size,
  const int d2Size,
  const distFuncType &distanceFunction,
  const diagFuncType &diagonalDistanceFunction,
  const double zeroThresh,
  std::vector<std::vector<double>> &minMatrix,
  std::vector<std::vector<double>> &maxMatrix,
  std::vector<std::vector<double>> &sadMatrix,
  const bool reverseMin,
  const bool reverseMax,
  const bool reverseSad,
  const int wasserstein) const {

  int maxI = 0, minI = 0;
  int maxJ = 0, minJ = 0;
  int sadI = 0, sadJ = 0;

  for(int i = 0; i < d1Size; ++i) {
    const auto &t1 = CTDiagram1[i];
    if(std::abs(std::get<4>(t1)) < zeroThresh)
      continue;

    const auto t11 = std::get<1>(t1);
    const auto t13 = std::get<3>(t1);
    bool isMin1 = (t11 == CriticalType::Local_minimum
                   || t13 == CriticalType::Local_minimum);
    bool isMax1 = (t11 == CriticalType::Local_maximum
                   || t13 == CriticalType::Local_maximum);
    bool isSad1
      = (t11 == CriticalType::Saddle1 && t13 == CriticalType::Saddle2)
        || (t11 == CriticalType::Saddle2 && t13 == CriticalType::Saddle1);
    if(t11 == CriticalType::Local_minimum
       && t13 == CriticalType::Local_maximum) {
      isMin1 = false;
      isMax1 = true;
    }

    minJ = 0;
    maxJ = 0;
    sadJ = 0;

    for(int j = 0; j < d2Size; ++j) {
      const auto &t2 = CTDiagram2[j];
      if(std::abs(std::get<4>(t2)) < zeroThresh)
        continue;

      const auto t21 = std::get<1>(t2);
      const auto t23 = std::get<3>(t2);
      bool isMin2 = (t21 == CriticalType::Local_minimum
                     || t23 == CriticalType::Local_minimum);
      bool isMax2 = (t21 == CriticalType::Local_maximum
                     || t23 == CriticalType::Local_maximum);
      bool isSad2
        = (t21 == CriticalType::Saddle1 && t23 == CriticalType::Saddle2)
          || (t21 == CriticalType::Saddle2 && t23 == CriticalType::Saddle1);
      if(t21 == CriticalType::Local_minimum
         && t23 == CriticalType::Local_maximum) {
        isMin2 = false;
        isMax2 = true;
      }
      if((isMin1 && !isMin2) || (isMax1 && !isMax2) || (isSad1 && !isSad2))
        continue;

      double distance = distanceFunction(t1, t2);
      double diag1 = diagonalDistanceFunction(t1);
      double diag2 = diagonalDistanceFunction(t2);

      if(distance > diag1 + diag2)
        distance = std::numeric_limits<double>::max();

      if(isMin1 && isMin2) {
        if(reverseMin)
          minMatrix[minJ++][minI] = distance;
        else
          minMatrix[minI][minJ++] = distance;
      } else if(isMax1 && isMax2) {
        if(reverseMax)
          maxMatrix[maxJ++][maxI] = distance;
        else
          maxMatrix[maxI][maxJ++] = distance;
      } else if(isSad1 && isSad2) {
        if(reverseSad)
          sadMatrix[sadJ++][sadI] = distance;
        else
          sadMatrix[sadI][sadJ++] = distance;
      }
    }

    double distanceToDiagonal = diagonalDistanceFunction(t1);
    if(isMin1) {
      if(reverseMin)
        minMatrix[minJ++][minI] = distanceToDiagonal;
      else
        minMatrix[minI][minJ++] = distanceToDiagonal;
    }
    if(isMax1) {
      if(reverseMax)
        maxMatrix[maxJ++][maxI] = distanceToDiagonal;
      else
        maxMatrix[maxI][maxJ++] = distanceToDiagonal;
    }
    if(isSad1) {
      if(reverseSad)
        sadMatrix[sadJ++][sadI] = distanceToDiagonal;
      else
        sadMatrix[sadI][sadJ++] = distanceToDiagonal;
    }

    if(isMin1)
      ++minI;
    if(isMax1)
      ++maxI;
    if(isSad1)
      ++sadI;
  }

  minJ = 0;
  maxJ = 0;
  sadJ = 0;

  // Last row: match remaining J components with diagonal.
  for(int j = 0; j < d2Size; ++j) {
    const auto &t2 = CTDiagram2[j];
    if(std::abs(std::get<4>(t2)) < zeroThresh)
      continue;

    const auto t21 = std::get<1>(t2);
    const auto t23 = std::get<3>(t2);
    bool isMin2 = (t21 == CriticalType::Local_minimum
                   || t23 == CriticalType::Local_minimum);
    bool isMax2 = (t21 == CriticalType::Local_maximum
                   || t23 == CriticalType::Local_maximum);
    bool isSad2
      = (t21 == CriticalType::Saddle1 && t23 == CriticalType::Saddle2)
        || (t21 == CriticalType::Saddle2 && t23 == CriticalType::Saddle1);
    if(t21 == CriticalType::Local_minimum
       && t23 == CriticalType::Local_maximum) {
      isMin2 = false;
      isMax2 = true;
    }

    double distanceToDiagonal = diagonalDistanceFunction(t2);
    if(isMin2) {
      if(reverseMin)
        minMatrix[minJ++][minI] = distanceToDiagonal;
      else
        minMatrix[minI][minJ++] = distanceToDiagonal;
    }
    if(isMax2) {
      if(reverseMax)
        maxMatrix[maxJ++][maxI] = distanceToDiagonal;
      else
        maxMatrix[maxI][maxJ++] = distanceToDiagonal;
    }
    if(isSad2) {
      if(reverseSad)
        sadMatrix[sadJ++][sadI] = distanceToDiagonal;
      else
        sadMatrix[sadI][sadJ++] = distanceToDiagonal;
    }
  }

  // Last cell
  {
    if(reverseMin)
      minMatrix[minJ][minI] = std::numeric_limits<double>::max();
    else
      minMatrix[minI][minJ] = std::numeric_limits<double>::max();
  }
  {
    if(reverseMax)
      maxMatrix[maxJ][maxI] = std::numeric_limits<double>::max();
    else
      maxMatrix[maxI][maxJ] = std::numeric_limits<double>::max();
  }
  {
    if(reverseSad)
      sadMatrix[sadJ][sadI] = std::numeric_limits<double>::max();
    else
      sadMatrix[sadI][sadJ] = std::numeric_limits<double>::max();
  }
}

double ttk::BottleneckDistance::computeGeometricalRange(
  const ttk::DiagramType &CTDiagram1,
  const ttk::DiagramType &CTDiagram2,
  const int d1Size,
  const int d2Size) const {
  float minX1, maxX1, minY1, maxY1, minZ1, maxZ1;
  float minX2, maxX2, minY2, maxY2, minZ2, maxZ2;
  float minX, minY, minZ, maxX, maxY, maxZ;
  minX1 = minY1 = minZ1 = minX2 = minY2 = minZ2
    = std::numeric_limits<float>::max();
  maxX1 = maxY1 = maxZ1 = maxX2 = maxY2 = maxZ2
    = std::numeric_limits<float>::min();

  for(int i = 0; i < d1Size; ++i) {
    const auto &t = CTDiagram1[i];
    float xa = std::get<7>(t), ya = std::get<8>(t), za = std::get<9>(t);
    float xb = std::get<11>(t), yb = std::get<12>(t), zb = std::get<13>(t);
    minX1 = std::min(std::min(minX1, xa), xb);
    minY1 = std::min(std::min(minY1, ya), yb);
    minZ1 = std::min(std::min(minZ1, za), zb);
    maxX1 = std::max(std::max(maxX1, xa), xb);
    maxY1 = std::max(std::max(maxY1, ya), yb);
    maxZ1 = std::max(std::max(maxZ1, za), zb);
  }

  for(int i = 0; i < d2Size; ++i) {
    const auto &t = CTDiagram2[i];
    float xa = std::get<7>(t), ya = std::get<8>(t), za = std::get<9>(t);
    float xb = std::get<11>(t), yb = std::get<12>(t), zb = std::get<13>(t);
    minX2 = std::min(std::min(minX2, xa), xb);
    minY2 = std::min(std::min(minY2, ya), yb);
    minZ2 = std::min(std::min(minZ2, za), zb);
    maxX2 = std::max(std::max(maxX2, xa), xb);
    maxY2 = std::max(std::max(maxY2, ya), yb);
    maxZ2 = std::max(std::max(maxZ2, za), zb);
  }

  minX = std::min(minX1, minX2);
  maxX = std::max(maxX1, maxX2);
  minY = std::min(minY1, minY2);
  maxY = std::max(maxY1, maxY2);
  minZ = std::min(minZ1, minZ2);
  maxZ = std::max(maxZ1, maxZ2);

  return std::sqrt(Geometry::pow(maxX - minX, 2) + Geometry::pow(maxY - minY, 2)
                   + Geometry::pow(maxZ - minZ, 2));
}

double ttk::BottleneckDistance::computeMinimumRelevantPersistence(
  const ttk::DiagramType &CTDiagram1,
  const ttk::DiagramType &CTDiagram2,
  const int d1Size,
  const int d2Size) const {

  double sp = zeroThreshold_;
  double s = sp > 0.0 && sp < 100.0 ? sp / 100.0 : 0;

  std::vector<double> toSort;
  for(int i = 0; i < d1Size; ++i) {
    const auto &t = CTDiagram1[i];
    const auto persistence = std::abs(std::get<4>(t));
    toSort.push_back(persistence);
  }
  for(int i = 0; i < d2Size; ++i) {
    const auto &t = CTDiagram2[i];
    const auto persistence = std::abs(std::get<4>(t));
    toSort.push_back(persistence);
  }
  sort(toSort.begin(), toSort.end());

  double minVal = toSort.at(0);
  double maxVal = toSort.at(toSort.size() - 1);
  s *= (maxVal - minVal);

  // double epsilon = 0.0000001;
  // int largeSize = 2000;
  // dataType zeroThresh = (dataType) epsilon;
  // if (d1Size + d2Size > largeSize + 1) {
  //   zeroThresh = toSort.at(d1Size + d2Size - largeSize);
  //   if (toSort.at(d1Size + d2Size - (largeSize+1)) == zeroThresh)
  //     zeroThresh += (dataType) epsilon;
  // }
  // if (zeroThresh < epsilon) zeroThresh = epsilon;

  return s;
}

void ttk::BottleneckDistance::computeMinMaxSaddleNumberAndMapping(
  const ttk::DiagramType &CTDiagram,
  int dSize,
  int &nbMin,
  int &nbMax,
  int &nbSaddle,
  std::vector<int> &minMap,
  std::vector<int> &maxMap,
  std::vector<int> &sadMap,
  const double zeroThresh) const {

  for(int i = 0; i < dSize; ++i) {
    const auto &t = CTDiagram[i];
    const auto nt1 = std::get<1>(t);
    const auto nt2 = std::get<3>(t);
    const auto dt = std::get<4>(t);
    if(std::abs(dt) < zeroThresh)
      continue;

    if(nt1 == CriticalType::Local_minimum
       && nt2 == CriticalType::Local_maximum) {
      nbMax++;
      maxMap.push_back(i);
    } else {
      if(nt1 == CriticalType::Local_maximum
         || nt2 == CriticalType::Local_maximum) {
        nbMax++;
        maxMap.push_back(i);
      }
      if(nt1 == CriticalType::Local_minimum
         || nt2 == CriticalType::Local_minimum) {
        nbMin++;
        minMap.push_back(i);
      }
      if((nt1 == CriticalType::Saddle1 && nt2 == CriticalType::Saddle2)
         || (nt1 == CriticalType::Saddle2 && nt2 == CriticalType::Saddle1)) {
        nbSaddle++;
        sadMap.push_back(i);
      }
    }
  }
}

void solvePWasserstein(const int nbRow,
                       const int nbCol,
                       std::vector<std::vector<double>> &matrix,
                       std::vector<MatchingType> &matchings,
                       ttk::AssignmentMunkres<double> &solver) {

  solver.setInput(matrix);
  solver.run(matchings);
  solver.clearMatrix();
}

void solveInfinityWasserstein(const int nbRow,
                              const int nbCol,
                              const int nbRowToCut,
                              const int nbColToCut,
                              std::vector<std::vector<double>> &matrix,
                              std::vector<MatchingType> &matchings,
                              ttk::GabowTarjan &solver) {

  // Copy input matrix.
  auto bottleneckMatrix = matrix;

  // Solve.
  solver.setInput(nbRow, nbCol, &bottleneckMatrix);
  solver.run(matchings);
  solver.clear();
}

double ttk::BottleneckDistance::buildMappings(
  const std::vector<MatchingType> &inputMatchings,
  const bool transposeGlobal,
  const bool transposeLocal,
  std::vector<MatchingType> &outputMatchings,
  const std::vector<int> &m1,
  const std::vector<int> &m2,
  int wasserstein) const {

  // Input map permutation (so as to ignore transposition later on)
  const std::vector<int> map1 = transposeLocal ? m2 : m1;
  const std::vector<int> map2 = transposeLocal ? m1 : m2;

  double addedPersistence = 0;
  for(int i = 0, s = (int)inputMatchings.size(); i < s; ++i) {
    const auto &t = inputMatchings.at(i);
    const auto val = std::abs(std::get<2>(t));

    int p1 = std::get<0>(t);
    int p2 = std::get<1>(t);

    if(p1 >= (int)map1.size() || p1 < 0 || p2 >= (int)map2.size() || p2 < 0) {
      addedPersistence = (wasserstein > 0 ? addedPersistence + val
                                          : std::max(val, addedPersistence));
    } else {
      int point1 = map1.at((unsigned long)p1);
      int point2 = map2.at((unsigned long)p2);
      bool doTranspose = transposeGlobal ^ transposeLocal;

      const auto &newT = doTranspose ? std::make_tuple(point2, point1, val)
                                     : std::make_tuple(point1, point2, val);

      outputMatchings.push_back(newT);
    }
  }

  return addedPersistence;
}

int ttk::BottleneckDistance::computeBottleneck(
  const ttk::DiagramType &d1,
  const ttk::DiagramType &d2,
  std::vector<MatchingType> &matchings,
  const bool usePersistenceMetric) {

  auto d1Size = (int)d1.size();
  auto d2Size = (int)d2.size();

  bool transposeOriginal = d1Size > d2Size;
  const ttk::DiagramType &CTDiagram1 = transposeOriginal ? d2 : d1;
  const ttk::DiagramType &CTDiagram2 = transposeOriginal ? d1 : d2;
  if(transposeOriginal) {
    int temp = d1Size;
    d1Size = d2Size;
    d2Size = temp;
  }

  if(transposeOriginal) {
    this->printMsg("The first persistence diagram is larger than the second.");
    this->printMsg("Solving the transposed problem.");
  }

  // Check user parameters.
  const int wasserstein = (wasserstein_ == "inf") ? -1 : stoi(wasserstein_);
  if(wasserstein < 0 && wasserstein != -1)
    return -4;

  // Needed to limit computation time.
  const auto zeroThresh = this->computeMinimumRelevantPersistence(
    CTDiagram1, CTDiagram2, d1Size, d2Size);

  // Initialize solvers.
  std::vector<MatchingType> minMatchings;
  std::vector<MatchingType> maxMatchings;
  std::vector<MatchingType> sadMatchings;

  // Initialize cost matrices.
  int nbRowMin = 0, nbColMin = 0;
  int maxRowColMin = 0, minRowColMin = 0;
  int nbRowMax = 0, nbColMax = 0;
  int maxRowColMax = 0, minRowColMax = 0;
  int nbRowSad = 0, nbColSad = 0;
  int maxRowColSad = 0, minRowColSad = 0;

  // Remap for matchings.
  std::vector<int> minMap1;
  std::vector<int> minMap2;
  std::vector<int> maxMap1;
  std::vector<int> maxMap2;
  std::vector<int> sadMap1;
  std::vector<int> sadMap2;

  this->computeMinMaxSaddleNumberAndMapping(CTDiagram1, d1Size, nbRowMin,
                                            nbRowMax, nbRowSad, minMap1,
                                            maxMap1, sadMap1, zeroThresh);
  this->computeMinMaxSaddleNumberAndMapping(CTDiagram2, d2Size, nbColMin,
                                            nbColMax, nbColSad, minMap2,
                                            maxMap2, sadMap2, zeroThresh);

  // Automatically transpose if nb rows > nb cols
  maxRowColMin = std::max(nbRowMin + 1, nbColMin + 1);
  maxRowColMax = std::max(nbRowMax + 1, nbColMax + 1);
  maxRowColSad = std::max(nbRowSad + 1, nbColSad + 1);

  minRowColMin = std::min(nbRowMin + 1, nbColMin + 1);
  minRowColMax = std::min(nbRowMax + 1, nbColMax + 1);
  minRowColSad = std::min(nbRowSad + 1, nbColSad + 1);

  std::vector<std::vector<double>> minMatrix(
    (unsigned long)minRowColMin, std::vector<double>(maxRowColMin));
  std::vector<std::vector<double>> maxMatrix(
    (unsigned long)minRowColMax, std::vector<double>(maxRowColMax));
  std::vector<std::vector<double>> sadMatrix(
    (unsigned long)minRowColSad, std::vector<double>(maxRowColSad));

  double px = px_;
  double py = py_;
  double pz = pz_;
  double pe = pe_;
  double ps = ps_;

  const auto distanceFunction
    = [wasserstein, px, py, pz, pe, ps](
        const ttk::PairTuple &a, const ttk::PairTuple &b) -> double {
    const auto ta1 = std::get<1>(a);
    const auto ta2 = std::get<3>(a);
    const int w = wasserstein > 1 ? wasserstein : 1; // L_inf not managed.

    // We don't match critical points of different index.
    // This must be ensured before calling the distance function.
    // const auto tb1 = get<1>(b);
    // const auto tb2 = get<3>(b);
    bool isMin1 = ta1 == CriticalType::Local_minimum;
    bool isMax1 = ta2 == CriticalType::Local_maximum;
    // bool isBoth = isMin1 && isMax1;

    const auto rX = std::get<6>(a);
    const auto rY = std::get<10>(a);
    const auto cX = std::get<6>(b);
    const auto cY = std::get<10>(b);
    const auto x
      = ((isMin1 && !isMax1) ? pe : ps) * Geometry::pow(std::abs(rX - cX), w);
    const auto y = (isMax1 ? pe : ps) * Geometry::pow(std::abs(rY - cY), w);
    double geoDistance
      = isMax1
          ? (px * Geometry::pow(abs(std::get<11>(a) - std::get<11>(b)), w)
             + py * Geometry::pow(abs(std::get<12>(a) - std::get<12>(b)), w)
             + pz * Geometry::pow(abs(std::get<13>(a) - std::get<13>(b)), w))
        : isMin1
          ? (px * Geometry::pow(abs(std::get<7>(a) - std::get<7>(b)), w)
             + py * Geometry::pow(abs(std::get<8>(a) - std::get<8>(b)), w)
             + pz * Geometry::pow(abs(std::get<9>(a) - std::get<9>(b)), w))
          : (px
               * Geometry::pow(abs(std::get<7>(a) + std::get<11>(a)) / 2
                                 - abs(std::get<7>(b) + std::get<11>(b)) / 2,
                               w)
             + py
                 * Geometry::pow(abs(std::get<8>(a) + std::get<12>(a)) / 2
                                   - abs(std::get<8>(b) + std::get<12>(b)) / 2,
                                 w)
             + pz
                 * Geometry::pow(abs(std::get<9>(a) + std::get<13>(a)) / 2
                                   - abs(std::get<9>(b) + std::get<13>(b)) / 2,
                                 w));

    double persDistance = x + y;
    double val = persDistance + geoDistance;
    val = Geometry::pow(val, 1.0 / w);
    return val;
  };

  const auto diagonalDistanceFunction
    = [wasserstein, px, py, pz, ps, pe](const ttk::PairTuple a) -> double {
    const auto ta1 = std::get<1>(a);
    const auto ta2 = std::get<3>(a);
    const int w = wasserstein > 1 ? wasserstein : 1;
    bool isMin1 = ta1 == CriticalType::Local_minimum;
    bool isMax1 = ta2 == CriticalType::Local_maximum;

    const auto rX = std::get<6>(a);
    const auto rY = std::get<10>(a);
    double x1 = std::get<7>(a);
    double y1 = std::get<8>(a);
    double z1 = std::get<9>(a);
    double x2 = std::get<11>(a);
    double y2 = std::get<12>(a);
    double z2 = std::get<13>(a);

    double infDistance
      = (isMin1 || isMax1 ? pe : ps) * Geometry::pow(std::abs(rX - rY), w);
    double geoDistance = (px * Geometry::pow(abs(x2 - x1), w)
                          + py * Geometry::pow(abs(y2 - y1), w)
                          + pz * Geometry::pow(abs(z2 - z1), w));
    double val = infDistance + geoDistance;
    return Geometry::pow(val, 1.0 / w);
  };

  const bool transposeMin = nbRowMin > nbColMin;
  const bool transposeMax = nbRowMax > nbColMax;
  const bool transposeSad = nbRowSad > nbColSad;

  Timer t;

  this->buildCostMatrices(
    CTDiagram1, CTDiagram2, d1Size, d2Size, distanceFunction,
    diagonalDistanceFunction, zeroThresh, minMatrix, maxMatrix, sadMatrix,
    transposeMin, transposeMax, transposeSad, wasserstein);

  if(wasserstein > 0) {

    if(nbRowMin > 0 && nbColMin > 0) {
      AssignmentMunkres<double> solverMin;
      this->printMsg("Affecting minima...");
      solvePWasserstein(
        minRowColMin, maxRowColMin, minMatrix, minMatchings, solverMin);
    }

    if(nbRowMax > 0 && nbColMax > 0) {
      AssignmentMunkres<double> solverMax;
      this->printMsg("Affecting maxima...");
      solvePWasserstein(
        minRowColMax, maxRowColMax, maxMatrix, maxMatchings, solverMax);
    }

    if(nbRowSad > 0 && nbColSad > 0) {
      AssignmentMunkres<double> solverSad;
      this->printMsg("Affecting saddles...");
      solvePWasserstein(
        minRowColSad, maxRowColSad, sadMatrix, sadMatchings, solverSad);
    }

  } else {

    // Launch solving for minima.
    if(nbRowMin > 0 && nbColMin > 0) {
      GabowTarjan solverMin;
      this->printMsg("Affecting minima...");
      solveInfinityWasserstein(minRowColMin, maxRowColMin, nbRowMin, nbColMin,
                               minMatrix, minMatchings, solverMin);
    }

    // Launch solving for maxima.
    if(nbRowMax > 0 && nbColMax > 0) {
      GabowTarjan solverMax;
      this->printMsg("Affecting maxima...");
      solveInfinityWasserstein(minRowColMax, maxRowColMax, nbRowMax, nbColMax,
                               maxMatrix, maxMatchings, solverMax);
    }

    // Launch solving for saddles.
    if(nbRowSad > 0 && nbColSad > 0) {
      GabowTarjan solverSad;
      this->printMsg("Affecting saddles...");
      solveInfinityWasserstein(minRowColSad, maxRowColSad, nbRowSad, nbColSad,
                               sadMatrix, sadMatchings, solverSad);
    }
  }

  this->printMsg("TTK CORE DONE", 1, t.getElapsedTime());

  // Rebuild mappings.
  // Begin cost computation for unpaired vertices.
  // std::cout << "Min" << std::endl;
  const auto addedMinPersistence
    = this->buildMappings(minMatchings, transposeOriginal, transposeMin,
                          matchings, minMap1, minMap2, wasserstein);

  // std::cout << "Max" << std::endl;
  const auto addedMaxPersistence
    = this->buildMappings(maxMatchings, transposeOriginal, transposeMax,
                          matchings, maxMap1, maxMap2, wasserstein);

  // std::cout << "Sad" << std::endl;
  const auto addedSadPersistence
    = this->buildMappings(sadMatchings, transposeOriginal, transposeSad,
                          matchings, sadMap1, sadMap2, wasserstein);

  // TODO [HIGH] do that for embeddings
  // Recompute matching weights for user-friendly distance.
  double d = 0;
  std::vector<bool> paired1(d1Size);
  std::vector<bool> paired2(d2Size);
  for(int b = 0; b < d1Size; ++b)
    paired1[b] = false;
  for(int b = 0; b < d2Size; ++b)
    paired2[b] = false;

  int numberOfMismatches = 0;
  for(int m = 0, ms = (int)matchings.size(); m < ms; ++m) {
    MatchingType mt = matchings[m];
    int i = transposeOriginal ? std::get<1>(mt) : std::get<0>(mt);
    int j = transposeOriginal ? std::get<0>(mt) : std::get<1>(mt);
    // dataType val = std::get<2>(t);

    const auto &t1 = CTDiagram1[i];
    const auto &t2 = CTDiagram2[j];
    // dataType rX = std::get<6>(t1); dataType rY = std::get<10>(t1);
    // dataType cX = std::get<6>(t2); dataType cY = std::get<10>(t2);
    // dataType x = rX - cX; dataType y = rY - cY;
    paired1[i] = true;
    paired2[j] = true;
    // dataType lInf = std::max(abs<dataType>(x), abs<dataType>(y));

    // if (((wasserstein < 0 && lInf != val) || (wasserstein > 0 && pow(lInf,
    // wasserstein) != val)))
    //++numberOfMismatches;

    auto partialDistance = distanceFunction(t1, t2);
    // wasserstein > 0 ? pow(lInf, wasserstein) : std::max(d, lInf);

    if(wasserstein > 0)
      d += partialDistance;
    else
      d = partialDistance;
  }

  if(numberOfMismatches > 0) {
    this->printWrn("Distance mismatch when rebuilding "
                   + std::to_string(numberOfMismatches) + " matchings");
  }

  auto affectationD = d;
  d = wasserstein > 0
        ? Geometry::pow(
          d + addedMaxPersistence + addedMinPersistence + addedSadPersistence,
          (1.0 / (double)wasserstein))
        : std::max(
          d, std::max(addedMaxPersistence,
                      std::max(addedMinPersistence, addedSadPersistence)));

  {
    std::stringstream msg;
    this->printMsg("Computed distance:");
    this->printMsg("diagMax(" + std::to_string(addedMaxPersistence)
                   + "), diagMin(" + std::to_string(addedMinPersistence)
                   + "), diagSad(" + std::to_string(addedSadPersistence) + ")");
    this->printMsg("affAll(" + std::to_string(affectationD) + "), res("
                   + std::to_string(d) + ")");
  }

  distance_ = d;
  return 0;
}
