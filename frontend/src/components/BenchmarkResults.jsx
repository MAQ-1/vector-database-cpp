const BenchmarkResults = ({ data, isLoading }) => {
  if (isLoading) {
    return (
      <div className="bg-[#101722] border border-white/[0.06] rounded-xl p-6 shadow-sm">
        <div className="flex items-center justify-between mb-6">
          <h3 className="text-lg font-semibold tracking-tight text-slate-100">Performance Benchmark</h3>
          <span className="text-xs font-mono text-violet-400/80">Running...</span>
        </div>
        <div className="flex flex-col items-center justify-center py-16 gap-4">
          <div className="h-8 w-8 border-2 border-violet-500/30 border-t-violet-400 rounded-full animate-spin"></div>
          <p className="text-sm text-slate-400">Comparing algorithm performance...</p>
        </div>
      </div>
    )
  }

  if (!data) {
    return (
      <div className="bg-[#101722] border border-white/[0.06] rounded-xl p-6 shadow-sm">
        <div className="flex items-center justify-between mb-6">
          <h3 className="text-lg font-semibold tracking-tight text-slate-100">Performance Benchmark</h3>
          <span className="text-xs font-mono text-slate-500">Not run</span>
        </div>
        <div className="flex flex-col items-center justify-center py-16 gap-3">
          <div className="size-10 rounded-lg bg-white/[0.03] border border-white/[0.06] flex items-center justify-center text-slate-600">
            <svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="1.5" strokeLinecap="round" strokeLinejoin="round" className="size-5">
              <polyline points="22 12 18 12 15 21 9 3 6 12 2 12"/>
            </svg>
          </div>
          <h4 className="text-sm font-medium text-slate-300">No benchmark data</h4>
          <p className="text-sm text-slate-500 max-w-md text-center">Click "COMPARE ALL ALGORITHMS" to run performance comparison</p>
        </div>
      </div>
    )
  }

  const algorithms = Object.keys(data)
  const maxTime = Math.max(...Object.values(data))
  const fastestAlgorithm = algorithms.reduce((fastest, algo) => 
    data[algo] < data[fastest] ? algo : fastest
  )

  return (
    <div className="bg-[#101722] border border-white/[0.06] rounded-xl shadow-sm overflow-hidden">
      {/* Benchmark Header */}
      <div className="flex items-center justify-between px-6 py-4 border-b border-white/[0.06]">
        <div className="flex items-center gap-3">
          <h3 className="text-lg font-semibold tracking-tight text-slate-100">Performance Benchmark</h3>
        </div>
        <div className="flex items-center gap-3">
          <span className="text-xs font-mono text-slate-500">Execution Time (μs)</span>
          <span className="text-slate-600">·</span>
          <span className="text-xs font-mono text-cyan-400/80">
            Fastest: {fastestAlgorithm}
          </span>
        </div>
      </div>

      {/* Benchmark Table */}
      <div className="overflow-x-auto">
        <table className="w-full">
          <thead>
            <tr className="border-b border-white/[0.06]">
              <th className="text-left px-6 py-3 text-[11px] font-medium uppercase tracking-wider text-slate-500">
                Algorithm
              </th>
              <th className="text-left px-4 py-3 text-[11px] font-medium uppercase tracking-wider text-slate-500">
                Execution Time
              </th>
              <th className="text-left px-4 py-3 text-[11px] font-medium uppercase tracking-wider text-slate-500">
                Relative Speed
              </th>
              <th className="text-left px-6 py-3 text-[11px] font-medium uppercase tracking-wider text-slate-500 w-[40%]">
                Performance
              </th>
            </tr>
          </thead>
          <tbody>
            {algorithms.map((algorithm) => {
              const time = data[algorithm]
              const percentage = (time / maxTime) * 100
              const isFastest = algorithm === fastestAlgorithm
              
              return (
                <tr 
                  key={algorithm} 
                  className={`border-b border-white/[0.04] hover:bg-white/[0.02] transition-colors duration-150 ${
                    isFastest ? 'bg-violet-500/[0.03]' : ''
                  }`}
                >
                  <td className="px-6 py-4">
                    <div className="flex items-center gap-2">
                      <span className={`text-sm font-medium ${isFastest ? 'text-violet-400' : 'text-slate-300'}`}>
                        {algorithm}
                      </span>
                      {isFastest && (
                        <span className="inline-flex items-center gap-1 text-[11px] text-emerald-400/80">
                          <span className="size-1 rounded-full bg-emerald-400/60"></span>
                          Fastest
                        </span>
                      )}
                    </div>
                  </td>
                  <td className="px-4 py-4">
                    <code className={`text-sm font-mono font-medium tabular-nums ${isFastest ? 'text-violet-400' : 'text-slate-200'}`}>
                      {time.toFixed(2)} μs
                    </code>
                  </td>
                  <td className="px-4 py-4">
                    <div className="flex items-center gap-2">
                      <span className="text-sm font-mono text-slate-300 tabular-nums">
                        {algorithm === fastestAlgorithm ? '1.00×' : `~${(time / data[fastestAlgorithm]).toFixed(0)}×`}
                      </span>
                      <span className="text-[11px] text-slate-500">
                        {algorithm === fastestAlgorithm ? 'baseline' : 'slower'}
                      </span>
                    </div>
                  </td>
                  <td className="px-6 py-4">
                    <div className="flex items-center gap-3">
                      <div className="flex-1 h-2 bg-white/[0.04] rounded-full overflow-hidden">
                        <div 
                          className={`h-full rounded-full transition-all duration-300 ${
                            isFastest ? 'bg-violet-500/50' : 'bg-slate-600/50'
                          }`}
                          style={{ width: `${Math.max(5, 100 - percentage)}%` }}
                        />
                      </div>
                      <span className="text-xs font-mono text-slate-500 tabular-nums min-w-[3rem] text-right">
                        {percentage.toFixed(1)}%
                      </span>
                    </div>
                  </td>
                </tr>
              )
            })}
          </tbody>
        </table>
      </div>

      {/* Performance Insights */}
      <div className="border-t border-white/[0.06] px-6 py-5">
        <h4 className="text-xs font-medium uppercase tracking-wider text-slate-500 mb-4">
          Performance Insights
        </h4>
        <div className="grid grid-cols-1 md:grid-cols-3 gap-4">
          <div className="flex flex-col gap-1.5 p-4 rounded-lg bg-white/[0.02] border border-white/[0.04]">
            <div className="flex items-center gap-2 mb-1">
              <div className="size-5 rounded bg-violet-500/10 flex items-center justify-center">
                <svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="1.5" strokeLinecap="round" strokeLinejoin="round" className="size-3 text-violet-400">
                  <polygon points="13 2 3 14 12 14 11 22 21 10 12 10 13 2"/>
                </svg>
              </div>
              <span className="text-xs font-medium text-slate-400">Fastest</span>
            </div>
            <span className="text-sm font-semibold text-violet-400">{fastestAlgorithm}</span>
            <span className="text-xs text-slate-500">
              achieved the lowest execution time at <span className="font-mono text-slate-300">{data[fastestAlgorithm].toFixed(2)}μs</span>
            </span>
          </div>

          <div className="flex flex-col gap-1.5 p-4 rounded-lg bg-white/[0.02] border border-white/[0.04]">
            <div className="flex items-center gap-2 mb-1">
              <div className="size-5 rounded bg-cyan-500/10 flex items-center justify-center">
                <svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="1.5" strokeLinecap="round" strokeLinejoin="round" className="size-3 text-cyan-400">
                  <line x1="18" y1="20" x2="18" y2="10"/>
                  <line x1="12" y1="20" x2="12" y2="4"/>
                  <line x1="6" y1="20" x2="6" y2="14"/>
                </svg>
              </div>
              <span className="text-xs font-medium text-slate-400">Range</span>
            </div>
            <span className="text-sm font-semibold text-cyan-400 font-mono">
              {data[fastestAlgorithm].toFixed(2)} → {maxTime.toFixed(2)}
            </span>
            <span className="text-xs text-slate-500">
              execution times span across all algorithms
            </span>
          </div>

          <div className="flex flex-col gap-1.5 p-4 rounded-lg bg-white/[0.02] border border-white/[0.04]">
            <div className="flex items-center gap-2 mb-1">
              <div className="size-5 rounded bg-emerald-500/10 flex items-center justify-center">
                <svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="1.5" strokeLinecap="round" strokeLinejoin="round" className="size-3 text-emerald-400">
                  <polyline points="20 6 9 17 4 12"/>
                </svg>
              </div>
              <span className="text-xs font-medium text-slate-400">Speed Difference</span>
            </div>
            <span className="text-sm font-semibold text-emerald-400 font-mono">
              ~{(maxTime / data[fastestAlgorithm]).toFixed(0)}×
            </span>
            <span className="text-xs text-slate-500">
              {fastestAlgorithm} is faster than the slowest algorithm
            </span>
          </div>
        </div>
      </div>

      {/* Legend */}
      <div className="border-t border-white/[0.06] px-6 py-4 flex items-center gap-6">
        <div className="flex items-center gap-1.5">
          <span className="size-2 rounded-full bg-violet-500/50"></span>
          <span className="text-xs text-slate-500">Fastest algorithm</span>
        </div>
        <div className="flex items-center gap-1.5">
          <span className="size-2 rounded-full bg-slate-600/50"></span>
          <span className="text-xs text-slate-500">Other algorithms</span>
        </div>
        <div className="flex items-center gap-2 ml-auto">
          <span className="text-[11px] text-slate-600">Faster</span>
          <div className="h-px w-16 bg-gradient-to-r from-violet-500/30 to-slate-600/30"></div>
          <span className="text-[11px] text-slate-600">Slower</span>
        </div>
      </div>
    </div>
  )
}

export default BenchmarkResults